.. title:: clang-tidy - performance-returning-type

performance-returning-type
==========================

This check finds places where we are returning object of a different type than
the function return type. In such places, we should use std::move, otherwise
the object will not be moved automatically.
Check adds std::move if it could be beneficial.

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
    return std::move(s);
  }

Of course if we return an rvalue (e.g., temporary) we don’t have to move it:

.. code-block:: c++

  boost::optional<std::string> get() {
    return "str";
  }
