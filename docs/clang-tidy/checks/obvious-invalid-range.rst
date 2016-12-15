.. title:: clang-tidy - obvious-invalid-range

obvious-invalid-range
=====================

This check finds invalid calls to algorithms with obviously invalid range of
iterators like:

.. code-block:: c++
    std::fill(v.begin(), v2.end(), it);

It checks if first and second argument of call is method call to ``begin()``
and ``end()``, but on different objects (having different name).

By default it looks for all algorithms from ``<algorithm>``. This can be
changed by using ``AlgorithmNames`` option, where empty list means that any
function name will be accepted.
