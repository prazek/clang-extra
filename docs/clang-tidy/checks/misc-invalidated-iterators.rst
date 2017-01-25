.. title:: clang-tidy - misc-invalidated-iterators

misc-invalidated-iterators
==========================

This check finds the possible invalidations of the iterators/references/pointers
through the container reallocation. If a programmer takes an iterator
to the element of the container and then twiddles with the container, it is
possible that the reference does not point to the original variable anymore.

Example:

.. code-block:: c++

  std::vector<int> vec{42};
  int &ref = vec[0];
  vec.push_back(2017);
  ref++;  // Incorrect - 'ref' might have been invalidated at 'push_back'.

