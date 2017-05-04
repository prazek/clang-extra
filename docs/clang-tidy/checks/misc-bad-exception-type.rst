.. title:: clang-tidy - misc-bad-exception-type

misc-bad-exception-type
=======================

Checks for exceptions of inappropriate type,
for example string, int or pointer.

It allows only record types and blacklists some types
from STL. In particular:

* std::string (and std::basic_string<T>)
* std::auto_ptr
* std::unique_ptr

See also misc-throw-by-value-catch-by-reference.

Example:

.. code-block:: c++

  throw 42;
  throw "42";
  throw std::string("42");
