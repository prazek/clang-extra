.. title:: clang-tidy - misc-use-bool-literals

misc-use-bool-literals
======================

Finds places, where integer literal is implict casted to boolean. 

.. code-block:: c++

  bool p = 1;
  std::ios_base::sync_with_stdio(0);

  // transforms to

  bool p = true;
  std::ios_base::sync_with_stdio(false);
