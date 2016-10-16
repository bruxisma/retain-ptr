#ifndef SG14_MEMORY_HPP
#define SG14_MEMORY_HPP

#include <type_traits>
#include <utility>
#include <atomic>

namespace sg14 {

using std::integral_constant;
using std::false_type;
using std::true_type;

using std::is_convertible;
using std::is_same;

using std::conditional_t;
using std::add_pointer_t;
using std::nullptr_t;

using std::declval;

} /* namespace sg14 */

namespace sg14 {
namespace impl {

template <bool B> using bool_constant = integral_constant<bool, B>;

template <class...> using void_t = void;
template <class T> struct identity { using type = T; };

template <class T, class Void, template <class...> class, class...>
struct detector : identity<T> { using value_t = false_type; };

template <class T, template <class...> class U, class... Args>
struct detector<T, void_t<U<Args...>>, U, Args...> :
  identity<U<Args...>>
{ using value_t = true_type; };

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
using is_detected_convertible = is_convertible<
  detected_t<T, Args...>,
  To
>;

template <class T, template <class...> class U, class... Args>
using is_detected_exact = is_same<T, detected_t<U, Args...>>;

template<class...> struct conjunction;
template<> struct conjunction<> : true_type { };
template<class B, class... Bs>
struct conjunction<B, Bs...> : conditional_t<
  B::value,
  conjunction<Bs...>,
  B
> { };

template <class T>
using has_pointer = typename T::pointer;

template <class T, class P>
using has_use_count = decltype(T::use_count(declval<P>()));

template <class T, class P, bool=is_detected<has_use_count, T, P>::value>
struct safe_count;

template <class T, class P> struct safe_count<T, P, true> :
  bool_constant<noexcept(T::use_count(declval<P>()))>
{ };

template <class T, class P> struct safe_count<T, P, false> : false_type { };

template <class T, class P>
using safe_increment = bool_constant<noexcept(T::increment(declval<P>()))>;

template <class T, class P>
using safe_decrement = bool_constant<noexcept(T::decrement(declval<P>()))>;

template <class T, class P>
using safe_incdec = conjunction<
  safe_increment<T, P>,
  safe_decrement<T, P>
>;

template <class T, class P>
using safe_use_count = conjunction<
  is_detected<has_use_count, T, P>,
  safe_count<T, P>
>;

}} /* namespace sg14::impl */

namespace sg14 {

using impl::detected_or_t;
using impl::is_detected;

template <class> struct retain_traits;

template <class Base, class Derived>
constexpr bool IsBaseOf = std::is_base_of<Base, Derived>::value;

template <class T>
struct atomic_reference_count {
  friend retain_traits<T>;
protected:
  atomic_reference_count () = default;
private:
  std::atomic<long> count { 1 };
};

template <class T>
struct reference_count {
  friend retain_traits<T>;
protected:
  reference_count () = default;
private:
  long count { 1 };
};

struct retain_t { constexpr retain_t () noexcept = default; };
constexpr retain_t retain { };

template <class T>
struct retain_traits final {

  static void increment (atomic_reference_count<T>* ptr) noexcept {
    ptr->count.fetch_add(1, std::memory_order_acq_rel);
  }

  static void decrement (atomic_reference_count<T>* ptr) noexcept {
    ptr->count.fetch_sub(1, std::memory_order_acq_rel);
    if (not use_count(ptr)) { delete static_cast<T*>(ptr); }
  }

  static long use_count (atomic_reference_count<T>* ptr) noexcept {
    return ptr->count.load(std::memory_order_acquire);
  }

  static void increment (reference_count<T>* ptr) noexcept { ++ptr->count; }
  static void decrement (reference_count<T>* ptr) noexcept {
    if (ptr->count -= 1) { delete static_cast<T*>(ptr); }
  }
  static long use_count (reference_count<T>* ptr) noexcept {
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

  static constexpr bool SafeIncrement = impl::safe_increment<
    traits_type,
    pointer
  >::value;

  static constexpr bool SafeDecrement = impl::safe_decrement<
    traits_type,
    pointer
  >::value;

  static constexpr bool SafeIncDec = impl::safe_incdec<
    traits_type,
    pointer
  >::value;

  static constexpr bool SafeUseCount = impl::safe_use_count<
    traits_type,
    pointer
  >::value;

  static constexpr auto has_use_count = is_detected<
    impl::has_use_count,
    traits_type,
    pointer
  > { };

  retain_ptr (pointer ptr, retain_t) noexcept(SafeIncrement) :
    retain_ptr { ptr }
  { if (*this) { traits_type::increment(this->get()); } }

  explicit retain_ptr (pointer ptr) : ptr { ptr } { }

  retain_ptr (nullptr_t) : retain_ptr { } { }

  retain_ptr (retain_ptr const& that) noexcept(SafeIncrement) :
    ptr { that.ptr }
  { if (*this) { traits_type::increment(this->get()); } }

  retain_ptr (retain_ptr&& that) noexcept :
    ptr { that.detach() }
  { }

  retain_ptr () noexcept = default;
  ~retain_ptr () noexcept(SafeDecrement) {
    if (*this) { traits_type::decrement(this->get()); }
  }

  retain_ptr& operator = (retain_ptr const& that) noexcept(SafeIncDec) {
    retain_ptr(that).swap(*this);
    return *this;
  }

  retain_ptr& operator = (retain_ptr&& that) noexcept(SafeDecrement) {
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

  long use_count () const noexcept(SafeUseCount) {
    return this->use_count(has_use_count);
  }
  bool unique () const noexcept(SafeUseCount) {
    return this->use_count() == 1;
  }

  pointer detach () noexcept {
    auto ptr = this->get();
    this->ptr = pointer { };
    return ptr;
  }

  void reset (pointer ptr, retain_t) noexcept(SafeIncDec) {
    *this = retain_ptr(ptr, retain);
  }

  void reset (pointer ptr) noexcept { retain_ptr(ptr).swap(*this); }

private:
  long use_count (true_type) const noexcept(SafeUseCount) {
    return this->get() ? traits_type::count(this->get()) : 0;
  }

  long use_count (false_type) const noexcept { return -1; }

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
bool operator >= (retain_ptr<T, R> const& lhs, nullptr_t) noexcept;

template <class T, class R>
bool operator <= (retain_ptr<T, R> const& lhs, nullptr_t) noexcept;

template <class T, class R>
bool operator > (retain_ptr<T, R> const& lhs, nullptr_t) noexcept;

template <class T, class R>
bool operator < (retain_ptr<T, R> const& lhs, nullptr_t) noexcept;

template <class T, class R>
bool operator == (nullptr_t, retain_ptr<T, R> const& rhs) noexcept;

template <class T, class R>
bool operator != (nullptr_t, retain_ptr<T, R> const& rhs) noexcept;

template <class T, class R>
bool operator >= (nullptr_t, retain_ptr<T, R> const& rhs) noexcept;

template <class T, class R>
bool operator <= (nullptr_t, retain_ptr<T, R> const& rhs) noexcept;

template <class T, class R>
bool operator > (nullptr_t, retain_ptr<T, R> const& rhs) noexcept;

template <class T, class R>
bool operator < (nullptr_t, retain_ptr<T, R> const& rhs) noexcept;

} /* namespace sg14 */

#endif /* SG14_MEMORY_HPP */
