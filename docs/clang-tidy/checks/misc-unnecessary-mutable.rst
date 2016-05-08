.. title:: clang-tidy - misc-unnecessary-mutable

misc-unnecessary-mutable
========================

The tool tries to find situations where ``mutable`` might be unnecessary
(i.e. each use of the mutable field on the const object is constant).

.. code-block:: c++
  class SomeClass {
    public:
      void foo() {
        field2 = 42;
      }

      void bar() const {
        field3 = 99;
      }

      // Should stay mutable. Everyone can change it.
      mutable int field1;

    private:
      // Might lose 'mutable'. It's never modified in const object.
      mutable int field2;

      // Should stay mutable; bar() changes it even though 'this' is const.
      mutable int field3;
  };

It also can automatically remove ``mutable`` keyword. For now, it is done
only if non-necessarily mutable field is the only declaration in the statement.

Please note that there is a possibility that false-positives occur. For example,
one can use ``const_cast``:

.. code-block:: c++
  class AnotherClass {
    public:
      void foo() const {
        const int& y = x;
        const_cast<int&>(y) = 42;
      }

    private:
      mutable int x; // SHOULD stay mutable. For now, it does not.
  };

The tool should be thus used with care.
