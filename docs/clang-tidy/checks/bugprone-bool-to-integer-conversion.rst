.. title:: clang-tidy - bugprone-bool-to-integer-conversion

bugprone-bool-to-integer-conversion
====================================

This check looks for implicit conversion from bool literals to integer types

.. code-block:: C++

  int a = false;
  vector<bool> v(true); // Makes vector of one element
  if (a == true) {}

  // Changes it to
  int a = 0;
  vector<bool> v(1); // Makes vector of one element
  if (a == 1) {}

Because bitfields packing are compiler dependent, check treats single-bit
bitfields as bools

.. code-block:: C++

  struct BitFields {
    BitFields() : notAFlag(true), b(false) {}
    unsigned notAFlag : 3;
    unsigned flag : 1;
  };

  // Changes to
  struct BitFields {
    BitFields() : notAFlag(1), b(false) {}
    unsigned notAFlag : 3;
    unsigned flag : 1;
  };

It turns out that the common bug is to have function returning only bools but having int as return type.
If check finds case like this then it function return type to bool.


.. code-block:: C++
  int fun() {
    return true;
    return fun();
    bool z;
    return z;
  }

  // changes it to
  bool fun() {
    return true;
    return fun();
    bool z;
    return z;
  }
