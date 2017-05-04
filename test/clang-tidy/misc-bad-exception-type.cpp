// RUN: %check_clang_tidy %s misc-bad-exception-type %t

void throw_int() {
  throw 42;
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (int) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type

namespace std {
  template <typename T>
  class basic_string {};

  using string = basic_string<char>;

  template <typename T>
  class auto_ptr {};

  template <typename T>
  class unique_ptr {};

  template <typename T>
  class shared_ptr {};

  template <typename T>
  class weak_ptr {};
}


void throw_string() {
  throw std::string();
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (std::string) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type

void throw_auto_ptr() {
  throw std::auto_ptr<int>();
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (std::auto_ptr<int>) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type

void throw_unique_ptr() {
  throw std::unique_ptr<int>();
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (std::unique_ptr<int>) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type

void throw_shared_ptr() {
  throw std::shared_ptr<int>();
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (std::shared_ptr<int>) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type

void throw_weak_ptr() {
  throw std::weak_ptr<int>();
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (std::weak_ptr<int>) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type


class A{};

void throw_class() {
  throw A();
}

void throw_pointer_to_class() {
  A a;
  throw &a;
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (class A *) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type


void throw_pointer_to_int() {
  int x;
  throw &x;
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (int *) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type

void throw_nullptr() {
  throw nullptr;
}
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: the type of the thrown exception (nullptr_t) is not appropriate; use a class indicating what happened instead [misc-bad-exception-type
