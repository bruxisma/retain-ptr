#ifndef SG14_MEMORY_HPP
#define SG14_MEMORY_HPP

#include <type_traits>
#include <functional>
#include <utility>
#include <memory>
#include <atomic>

namespace sg14 {

using std::is_convertible;
using std::is_same;

using std::conditional_t;
using std::add_pointer_t;
using std::nullptr_t;

using std::declval;

} /* namespace sg14 */

namespace sg14 {
namespace impl {

template <class T> struct identity { using type = T; };

template <class T, class Void, template <class...> class, class...>
struct detector : identity<T> { using value_t = std::false_type; };

template <class T, template <class...> class U, class... Args>
struct detector<T, std::void_t<U<Args...>>, U, Args...> :
  identity<U<Args...>>
{ using value_t = std::true_type; };

struct nonesuch final {
  nonesuch (nonesuch const&) = delete;
  nonesuch () = delete;
  ~nonesuch () = delete;

  void operator = (nonesuch const&) = delete;
};

template <class T, template <class...> class U, class... Args>
using detected_or = detector<T, void, U, Args...>;

template <template <class...> class T, class... Args>
using detected_t = typename detected_or<nonesuch, T, Args...>::type;

template <class T, template <class...> class U, class... Args>
using detected_or_t = typename detected_or<T, U, Args...>::type;

template <template <class...> class T, class... Args>
using is_detected = typename detected_or<nonesuch, T, Args...>::value_t;

template <class To, template <class...> class T, class... Args>
using is_detected_convertible = std::is_convertible<
  detected_t<T, Args...>,
  To
>;

template <class T, template <class...> class U, class... Args>
using is_detected_exact = std::is_same<T, detected_t<U, Args...>>;

template <class T>
using has_pointer = typename T::pointer;

template <class T>
using has_default_action = typename T::default_action;

template <class T, class P>
using has_use_count = decltype(T::use_count(std::declval<P>()));

}} /* namespace sg14::impl */

namespace sg14 {

using impl::detected_or_t;
using impl::is_detected;

template <class> struct retain_traits;

template <class T>
struct atomic_reference_count {
  template <class> friend class retain_traits;
protected:
  atomic_reference_count () = default;
private:
  std::atomic<long> count { 1 };
};

template <class T>
struct reference_count {
  template <class> friend class retain_traits;
protected:
  reference_count () = default;
private:
  long count { 1 };
};

struct retain_object_t {  retain_object_t () noexcept = default; };
struct adopt_object_t {  adopt_object_t () noexcept = default; };

constexpr retain_object_t retain_object { };
constexpr adopt_object_t adopt_object { };

template <class T>
struct retain_traits final {

  template <class U>
  static void increment (atomic_reference_count<U>* ptr) noexcept {
    ptr->count.fetch_add(1, std::memory_order_relaxed);
  }

  template <class U>
  static void decrement (atomic_reference_count<U>* ptr) noexcept {
    ptr->count.fetch_sub(1, std::memory_order_acq_rel);
    if (not use_count(ptr)) { delete static_cast<T*>(ptr); }
  }

  template <class U>
  static long use_count (atomic_reference_count<U>* ptr) noexcept {
    return ptr->count.load(std::memory_order_relaxed);
  }

  template <class U>
  static void increment (reference_count<U>* ptr) noexcept { ++ptr->count; }
  template <class U>
  static void decrement (reference_count<U>* ptr) noexcept {
    if (ptr->count -= 1) { delete static_cast<T*>(ptr); }
  }
  template <class U>
  static long use_count (reference_count<U>* ptr) noexcept {
    return ptr->count;
  }
};

template <class T, class R=retain_traits<T>>
struct retain_ptr {
  using element_type = T;
  using traits_type = R;

  using pointer = detected_or_t<
    add_pointer_t<element_type>,
    impl::has_pointer,
    traits_type
  >;

  using default_action = detected_or_t<
    adopt_object_t,
    impl::has_default_action,
    traits_type
  >;

  static constexpr bool CheckAction = std::disjunction_v<
    std::is_same<default_action, adopt_object_t>,
    std::is_same<default_action, retain_object_t>
  >;

  static_assert(
    CheckAction,
    "traits_type::default_action must be adopt_object_t or retain_object_t");

  static constexpr auto has_use_count = is_detected<
    impl::has_use_count,
    traits_type,
    pointer
  > { };

  retain_ptr (pointer ptr, retain_object_t) :
    retain_ptr { ptr, adopt_object }
  { if (*this) { traits_type::increment(this->get()); } }

  retain_ptr (pointer ptr, adopt_object_t) : ptr { ptr } { }

  explicit retain_ptr (pointer ptr) :
    retain_ptr { ptr, default_action() }
  { }

  retain_ptr (nullptr_t) : retain_ptr { } { }

  retain_ptr (retain_ptr const& that) :
    ptr { that.ptr }
  { if (*this) { traits_type::increment(this->get()); } }

  retain_ptr (retain_ptr&& that) noexcept :
    ptr { that.detach() }
  { }

  retain_ptr () noexcept = default;
  ~retain_ptr () {
    if (*this) { traits_type::decrement(this->get()); }
  }

  retain_ptr& operator = (retain_ptr const& that) {
    retain_ptr(that).swap(*this);
    return *this;
  }

  retain_ptr& operator = (retain_ptr&& that) {
    retain_ptr(std::move(that)).swap(*this);
    return *this;
  }

  retain_ptr& operator = (nullptr_t) noexcept { this->reset(); return *this; }

  void swap (retain_ptr& that) noexcept {
    using std::swap;
    swap(this->ptr, that.ptr);
  }

  explicit operator bool () const noexcept { return this->get(); }
  decltype(auto) operator * () const noexcept { return *this->get(); }
  pointer operator -> () const noexcept { return this->get(); }

  pointer get () const noexcept { return this->ptr; }

  long use_count () const {
    if constexpr (has_use_count) {
      return this->get() ? traits_type::use_count(this->get()) : 0;
    } else { return -1; }
  }

  pointer detach () noexcept {
    auto ptr = this->get();
    this->ptr = pointer { };
    return ptr;
  }

  void reset (pointer ptr, retain_object_t) {
    *this = retain_ptr(ptr, retain_object);
  }

  void reset (pointer ptr, adopt_object_t) noexcept {
    *this = retain_ptr(ptr, adopt_object);
  }

  void reset (pointer ptr) { *this = retain_ptr(ptr, default_action()); }

private:
  pointer ptr { };
};

template <class T, class R>
void swap (retain_ptr<T, R>& lhs, retain_ptr<T, R>& rhs) noexcept {
  lhs.swap(rhs);
}

template <class T, class R>
bool operator == (
  retain_ptr<T, R> const& lhs,
  retain_ptr<T, R> const& rhs
) noexcept { return lhs.get() == rhs.get(); }

template <class T, class R>
bool operator != (
  retain_ptr<T, R> const& lhs,
  retain_ptr<T, R> const& rhs
) noexcept { return lhs.get() != rhs.get(); }

template <class T, class R>
bool operator >= (
  retain_ptr<T, R> const& lhs,
  retain_ptr<T, R> const& rhs
) noexcept { return lhs.get() >= rhs.get(); }

template <class T, class R>
bool operator <= (
  retain_ptr<T, R> const& lhs,
  retain_ptr<T, R> const& rhs
) noexcept { return lhs.get() <= rhs.get(); }

template <class T, class R>
bool operator > (
  retain_ptr<T, R> const& lhs,
  retain_ptr<T, R> const& rhs
) noexcept { return lhs.get() > rhs.get(); }

template <class T, class R>
bool operator < (
  retain_ptr<T, R> const& lhs,
  retain_ptr<T, R> const& rhs
) noexcept { return lhs.get() < rhs.get(); }

template <class T, class R>
bool operator == (retain_ptr<T, R> const& lhs, nullptr_t) noexcept {
  return bool(lhs);
}

template <class T, class R>
bool operator != (retain_ptr<T, R> const& lhs, nullptr_t) noexcept {
  return not lhs;
}

template <class T, class R>
bool operator >= (retain_ptr<T, R> const& lhs, nullptr_t) noexcept {
  return not (lhs < nullptr);
}

template <class T, class R>
bool operator <= (retain_ptr<T, R> const& lhs, nullptr_t) noexcept {
  return nullptr < lhs;
}

template <class T, class R>
bool operator > (retain_ptr<T, R> const& lhs, nullptr_t) noexcept {
  return not (nullptr < lhs);
}

template <class T, class R>
bool operator < (retain_ptr<T, R> const& lhs, nullptr_t) noexcept {
  return std::less<>()(lhs.get(), nullptr);
}

template <class T, class R>
bool operator == (nullptr_t, retain_ptr<T, R> const& rhs) noexcept {
  return not rhs;
}

template <class T, class R>
bool operator != (nullptr_t, retain_ptr<T, R> const& rhs) noexcept {
  return bool(rhs);
}

template <class T, class R>
bool operator >= (nullptr_t, retain_ptr<T, R> const& rhs) noexcept {
  return not (nullptr < rhs);
}

template <class T, class R>
bool operator <= (nullptr_t, retain_ptr<T, R> const& rhs) noexcept;

template <class T, class R>
bool operator > (nullptr_t, retain_ptr<T, R> const& rhs) noexcept {
  return not (rhs < nullptr);
}

template <class T, class R>
bool operator < (nullptr_t, retain_ptr<T, R> const& rhs) noexcept {
  return std::less<>()(rhs.get(), nullptr);
}

} /* namespace sg14 */

#endif /* SG14_MEMORY_HPP */
