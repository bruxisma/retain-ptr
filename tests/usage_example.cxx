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

class Base: public sg14::reference_count<Base>, public instance_counted<Base>
{};

TEST_CASE("base class")
{
  using BasePtr = sg14::retain_ptr<Base>;
  {
    BasePtr ptr{new Base};
    REQUIRE(Base::numInstances == 1);
    REQUIRE(ptr.use_count() == 1);
    {
      BasePtr ptr2{ptr};
      REQUIRE(Base::numInstances == 1);
      REQUIRE(ptr.use_count() == 2);
      BasePtr pt3{std::move(ptr2)};
      REQUIRE(Base::numInstances == 1);
      REQUIRE(ptr.use_count() == 2);
    }
    REQUIRE(Base::numInstances == 1);
    REQUIRE(ptr.use_count() == 1);
  }
  REQUIRE(Base::numInstances == 0);
}

class Derived: public Base
{};

TEST_CASE("derived class")
{
  using DerivedPtr = sg14::retain_ptr<Derived>;
  {
    DerivedPtr ptr{new Derived};
    REQUIRE(Derived::numInstances == 1);
    REQUIRE(ptr.use_count() == 1);
    {
      DerivedPtr ptr2{ptr};
      REQUIRE(Derived::numInstances == 1);
      REQUIRE(ptr.use_count() == 2);
      DerivedPtr pt3{std::move(ptr2)};
      REQUIRE(Derived::numInstances == 1);
      REQUIRE(ptr.use_count() == 2);
    }
    REQUIRE(Derived::numInstances == 1);
    REQUIRE(ptr.use_count() == 1);
  }
  REQUIRE(Derived::numInstances == 0);
}
