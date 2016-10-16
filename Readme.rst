Overview
========

This is a proposal (P0468_) and reference implementation for an intrusive smart
pointer for the C++ standard library. The example provided is fragile, as it is
only being written against Python 3.5. More examples will eventually follow.

The implementation provided works with C++14, but could be backported to C++11.
Due to the feature lockin of C++17, this will most likely not make it into the
standard until C++20.

Creating the HTML file for the proposal requires ``rst2html.py``, which is a
part of the python ``docutils`` package.

The reference implementation (everything under the include directory) is under
the BSD 2-clause.

.. _P0468: https://wg21.link/p0468
