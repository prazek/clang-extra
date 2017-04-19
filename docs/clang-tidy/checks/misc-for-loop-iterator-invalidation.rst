.. title:: clang-tidy - misc-for-loop-iterator-invalidation

misc-for-loop-iterator-invalidation
==========================

Finds suspicious method calls on objects iterated using
C++11 for-range loop. If such call can invalidate iterators then
behaviour is undefined.

.. code-block:: c++

  std::vector<int> v = {1, 2, 3};
  for (auto i : v) {
    v.push_back(0); // Boom!
  }

