.. title:: clang-tidy - misc-for-loop-iterator-invalidation

misc-for-loop-iterator-invalidation
==========================

Finds suspicious method calls on objects iterated using
C++11 for-range loop. If such call can invalidate iterators then
the behaviour is undefined.

.. code-block:: c++

  std::vector<int> v = {1, 2, 3};
  for (auto i : v) {
    v.push_back(0); // Boom!
  }

The following heuristics is used to tell if a call is suspicious:

Function is likely to invalidate iterators if this function
is not const and container doesn't have a function with the same name, same
parameters (except 'this' parameter) but marked as const.

If the type of iterated object is on :option:`SafeTypes` list then call is ignored.

If after call execution will not come back to loop then call is ignored. It means that
following examples will not emit warning:

.. code-block:: c++

  std::vector<int> v = {1, 2, 3};
  for (auto i : v) {
    if (i == 2) {
      v.push_back(0);
      break;
    }
  }

.. code-block:: c++

  std::vector<int> v = {1, 2, 3};
  for (auto i : v) {
    if (i == 2) {
      v.push_back(0);
      return;
    }
  }

Options
-------

.. option:: SafeTypes

   Semicolon-separated list of names of types that should be considered safe. By default equal to
   ``::std::list; ::std::forward_list``.
