#include "doctest.hpp"
#include <sg14/memory.hpp>

template<class T>
struct instance_counted
{
  static long numInstances;
  instance_counted()
  {
    numInstances++;
  }
  ~instance_counted()
  {
    numInstances--;
  }
  instance_counted(const instance_counted&)
  {
    numInstances++;
  }
  instance_counted& operator=(const instance_counted&)
  {
    numInstances++;
  }
};

template<class T>
long instance_counted<T>::numInstances = 0;

template<class T>
void test_basic_usage()
{
  using TPtr = sg14::retain_ptr<T>;
  {
    TPtr ptr{new T};
    REQUIRE(T::numInstances == 1);
    REQUIRE(ptr.use_count() == 1);
    {
      TPtr ptr2{ptr};
      REQUIRE(T::numInstances == 1);
      REQUIRE(ptr.use_count() == 2);
      TPtr pt3{std::move(ptr2)};
      REQUIRE(T::numInstances == 1);
      REQUIRE(ptr.use_count() == 2);
    }
    REQUIRE(T::numInstances == 1);
    REQUIRE(ptr.use_count() == 1);
  }
  REQUIRE(T::numInstances == 0);
}

class Base: public sg14::reference_count<Base>, public instance_counted<Base>
{};

TEST_CASE("base class")
{
  test_basic_usage<Base>();
}

class Derived: public Base
{};

TEST_CASE("derived class")
{
  test_basic_usage<Derived>();
}

class ThreadSafeBase: public sg14::atomic_reference_count<ThreadSafeBase>, public instance_counted<ThreadSafeBase>
{};

TEST_CASE("thread safe base class")
  {
  test_basic_usage<ThreadSafeBase>();
}

class ThreadSafeDerived: public ThreadSafeBase
{};

TEST_CASE("thread safe derived class")
{
  test_basic_usage<ThreadSafeDerived>();
}
