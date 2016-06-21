.. title:: clang-tidy - performance-return-value-copy

performance-return-value-copy
==========================

Adds `std::move` in returns statements where returned object is copied and
adding `std::move` can make it being moved.

This check requires C++11 or higher to run.

For returning object of type A in function with return type B
check triggers if:
- B is move constructible from A
- B is constructible from value of A and A is movable.

check doesn't trigger when
- A is same type as B
- temporary object is returned
- returned object was declared as const or as reference
- A already has rvalue reference type

For example:

.. code-block:: c++

  boost::optional<std::string> get() {
    string s;
    ;;;
    return s;
  }

  // becomes

  boost::optional<std::string> get() {
    string s;
    ;;;
    return boost::optional<std::string>(std::move(s));
  }

Of course if we return an rvalue (e.g., temporary) we don’t have to move it:

.. code-block:: c++

  boost::optional<std::string> get() {
    return "str";
  }
