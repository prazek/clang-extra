.. title:: clang-tidy - modernize-increment-bool

modernize-increment-bool
========================

This check detects the deprecated boolean type incrementation, and wherever possible,
converts it into truth assignment.

C++17 forbids this behavior. Before C++17 this was equivalent to setting the variable
to true.


Example
-------

Original:

.. code-block:: c++

  bool variable = false;
  variable++;
  ++variable;

  bool another = ++variable;
  bool third = ++variable && another;

After applying the check:

.. code-block:: c++

  bool variable = false;
  variable = true;
  variable = true;  /* Both postfix and prefix incrementations are supported. */

  bool another = (variable = true);
  bool third = (variable = true) && another;


Limitations
-----------

When postfix boolean incrementation is not the outermost operation done in the instruction,
tool will not repair the problem (the fix would have to append some instructions after the
statement). For example:

.. code-block:: c++

  bool first = false, second;
  second = first++;

  // Equivalent to:
  second = false;
  first = true;

