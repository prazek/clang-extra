.. title:: clang-tidy - readability-for-should-be-while

readability-for-should-be-while
===============================

Finds for loops that can be expressed more naturally as while.

.. code-block:: c++

  for (;x < n;) {
  }

  for (;;) {
  }
