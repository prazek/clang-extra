// RUN: %check_clang_tidy %s performance-return-value-copy %t
// CHECK-FIXES: {{^}}#include <utility>{{$}}

// we need std::move mock
namespace std {
template <typename _Tp>
struct remove_reference { typedef _Tp type; };

template <typename _Tp>
struct remove_reference<_Tp &> { typedef _Tp type; };

template <typename _Tp>
struct remove_reference<_Tp &&> { typedef _Tp type; };

template <typename _Tp>
constexpr typename std::remove_reference<_Tp>::type &&
move(_Tp &&__t) noexcept { return static_cast<typename std::remove_reference<_Tp>::type &&>(__t); }
}

class SimpleClass {
public:
  SimpleClass() = default;
  SimpleClass(const SimpleClass &) = default;
  SimpleClass(SimpleClass &&) = default;

  // We don't want to add std::move here because it will be added by compiler
  SimpleClass foo(SimpleClass a, const SimpleClass b, SimpleClass &c, const SimpleClass &d, SimpleClass &&e, const SimpleClass &&f, char k) {
    switch (k) {
    case 'a':
      return a;
    case 'b':
      return b;
    case 'c':
      return c;
    case 'd':
      return d;
    case 'e':
      return e;
    case 'f':
      return f;
    default:
      return SimpleClass();
    }
  }
};

SimpleClass simpleClassFoo() {
  return SimpleClass();
}

class FromValueClass {
public:
  FromValueClass(SimpleClass a) {}

  FromValueClass foo(SimpleClass a, const SimpleClass b, SimpleClass &c, const SimpleClass &d, SimpleClass &&e, const SimpleClass &&f, char k) {
    switch (k) {
    case 'a':
      // Because SimpleClass is move constructible
      return a;
    // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: returned object is not moved; consider wrapping it with std::move or changing return type to avoid the copy [performance-return-value-copy]
    // CHECK-FIXES: {{^ *}}return FromValueClass(std::move(a));{{$}}
    case 'b':
      return b;
    case 'c':
      return c;
    case 'd':
      return d;
    case 'e':
      return e;
    case 'f':
      return f;
    case 'g':
      return simpleClassFoo();
    default:
      return SimpleClass();
    }
  }
};

class FromRRefClass {
public:
  FromRRefClass() = default;
  FromRRefClass(const SimpleClass &a) {}
  FromRRefClass(SimpleClass &&a) {}

  FromRRefClass foo1(SimpleClass a, const SimpleClass b, SimpleClass &c, const SimpleClass &d, SimpleClass &&e, const SimpleClass &&f, char k) {
    switch (k) {
    case 'a':
      return a;
    // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: {{..}}
    // CHECK-FIXES: {{^ *}}return FromRRefClass(std::move(a));{{$}}

    // We don't want to add std::move in cases 'b-f because
    case 'b':
      return b;
    case 'c':
      return c;
    case 'd':
      return d;
    case 'e':
      return e;
    case 'f':
      return f;
    case 'g':
      return simpleClassFoo();
    default:
      return SimpleClass();
    }
  }

  FromRRefClass foo2(char k) {
    SimpleClass a;
    const SimpleClass &b = a;
    SimpleClass &c = a;
    SimpleClass *d = &a;
    const SimpleClass e;

    switch (k) {
    case 'a':
      return a;
    // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: {{..}}
    // CHECK-FIXES: {{^ *}}return FromRRefClass(std::move(a));{{$}}
    case 'b':
      return b;
    case 'c':
      return c;
    case 'd':
      return *d;
    // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: {{..}}
    // CHECK-FIXES: {{^ *}}return FromRRefClass(std::move(*d));{{$}}
    case 'e':
      return e;
    case 'f':
      return simpleClassFoo();
    case 'x':
      return SimpleClass();
    case 'y':
      return FromRRefClass(SimpleClass());
    }
  }

  FromRRefClass foo3(char k) {
    SimpleClass a;
    SimpleClass b;
    FromRRefClass c;
    switch (k) {
    case 'a':
      return std::move(a);
    case 'b':
      return FromRRefClass(std::move(a));
    case 'c':
      return c;
    default:
      return FromRRefClass();
    }
  }
};

template <typename T>
FromRRefClass justTemplateFunction(T &&t) {
  return t;
}

void call_justTemplateFunction() {
  justTemplateFunction(SimpleClass{});
  SimpleClass a;
  justTemplateFunction(a);
  justTemplateFunction(FromRRefClass{});
  FromRRefClass b;
  justTemplateFunction(b);
}

class FromValueWithDeleted {
public:
  FromValueWithDeleted() = default;
  FromValueWithDeleted(SimpleClass a) {}
  FromValueWithDeleted(SimpleClass &&a) = delete;
};

FromValueWithDeleted foo9() {
  SimpleClass a;
  return a;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: {{..}}
  // CHECK-FIXES: {{^ *}}return FromValueWithDeleted(std::move(a));{{$}}
}

template <typename T>
struct old_optional {
  old_optional(const T &t) {}
};

old_optional<SimpleClass> test_old_optional() {
  SimpleClass a;
  return a;
}

template <typename T>
struct optional {
  optional(const T &) {}
  optional(T &&) {}
};

optional<SimpleClass> test_optional() {
  SimpleClass a;
  return a;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: {{..}}
  // CHECK-FIXES: {{^ *}}return optional<SimpleClass>(std::move(a));{{$}}
}

using Opt = optional<SimpleClass>;
Opt test_Opt() {
  SimpleClass a;
  return a;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: {{..}}
  // CHECK-FIXES: {{^ *}}return Opt(std::move(a));{{$}}
}

struct any {
  any() = default;

  template <typename T>
  any(T &&) {}

  any(any &&) = default;
  any(const any &) = default;
};

any test_any() {
  SimpleClass a;
  return a;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: {{..}}
  // CHECK-FIXES: {{^ *}}return any(std::move(a));{{$}}
}

any test_any2() {
  SimpleClass a;
  return std::move(a);
}
any test_any3() {
  SimpleClass a;
  return any(std::move(a));
}

any test_any4() {
  any a;
  return a;
}

class FromRefWithDeleted {
public:
  FromRefWithDeleted(SimpleClass &&) = delete;
  FromRefWithDeleted(const SimpleClass &a) {}
};

FromRefWithDeleted foo11(SimpleClass a) {
  return a;
}

class ClassWithUsings {
public:
  using value = SimpleClass;
  using const_value = const SimpleClass;
  using lreference = SimpleClass &;
  using const_lreference = const SimpleClass &;
  using rreference = SimpleClass &&;
  using const_rreference = const SimpleClass &&;

  ClassWithUsings(rreference) {}
  ClassWithUsings(const_lreference) {}

  ClassWithUsings foo(value a, const_value b, lreference c, const_lreference d, rreference e, const_rreference f, char k) {
    switch (k) {
    case 'a':
      return a;
    // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: {{..}}
    // CHECK-FIXES: {{^ *}}return ClassWithUsings(std::move(a));{{$}}
    case 'b':
      return b;
    case 'c':
      return c;
    case 'd':
      return d;
    case 'e':
      return e;
    case 'f':
      return f;
    case 'g':
      return simpleClassFoo();
    default:
      return SimpleClass();
    }
  }
};

class FromRRefWithDefaultArgs {
public:
  FromRRefWithDefaultArgs(SimpleClass &&, int k = 0) {}
  FromRRefWithDefaultArgs(const SimpleClass &) {}

  FromRRefWithDefaultArgs foo(SimpleClass a) {
    return a;
    // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: {{..}}
    // CHECK-FIXES: {{^ *}}return FromRRefWithDefaultArgs(std::move(a));{{$}}
  }
};
