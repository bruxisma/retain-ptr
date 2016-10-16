#ifndef PYTHON_WRAPPER_EXAMPLE_HPP
#define PYTHON_WRAPPER_EXAMPLE_HPP

#include <stdexcept>

#include <sg14/memory.hpp>
#include <Python.h>

namespace py {

struct snake_traits final {
  static void increment (PyObject* obj) noexcept { Py_INCREF(obj); }
  static void decrement (PyObject* obj) noexcept { Py_DECREF(obj); }
};

using object = sg14::retain_ptr<PyObject, snake_traits>;

struct tuple {
  using index_type = Py_ssize_t;

  tuple (object obj) noexcept(false) : obj { obj } {
    if (not PyTuple_Check(obj.get())) {
      throw std::logic_error("Was not passed a tuple");
    }
  }

  tuple (object::pointer obj) : tuple(object(obj)) { }
  tuple (index_type n) :
    tuple(PyTuple_New(n))
  { }

  object operator [] (index_type idx) const noexcept {
    return { PyTuple_GetItem(this->obj.get(), idx), sg14::retain };
  }

  index_type size () const noexcept { return PyTuple_Size(this->obj.get()); }

  tuple slice (index_type low, index_type high) const noexcept {
    return PyTuple_GetSlice(this->obj.get(), low, high);
  }

private:
  object obj;
};

struct list {
  using index_type = Py_ssize_t;

  list (object obj) noexcept(false) : obj { obj } {
    if (not PyList_Check(obj.get())) {
      throw std::logic_error("Was not passed a list");
    }
  }
  list (object::pointer obj) : list(object(obj)) { }
  list (index_type n) :
    list(PyList_New(n))
  { }

  object operator [] (index_type idx) const noexcept {
    return { PyList_GetItem(this->obj.get(), idx), sg14::retain };
  }

  explicit operator tuple () const noexcept {
    return PyList_AsTuple(this->obj.get());
  }

  void insert (index_type idx, object item) noexcept {
    PyList_Insert(this->obj.get(), idx, item.get());
  }

  void append (object item) noexcept {
    PyList_Append(this->obj.get(), item.get());
  }

  index_type size () const noexcept {
    return PyList_Size(this->obj.get());
  }

  void slice (index_type low, index_type high, list items) noexcept {
    PyList_SetSlice(this->obj.get(), low, high, items.obj.get());
  }

  list slice (index_type low, index_type high) const noexcept {
    return PyList_GetSlice(this->obj.get(), low, high);
  }

  void reverse () noexcept { PyList_Reverse(this->obj.get()); }
  void sort () noexcept { PyList_Sort(this->obj.get()); }

private:
  object obj;
};

} /* namespace py */

#endif /* PYTHON_WRAPPER_EXAMPLE_HPP */
