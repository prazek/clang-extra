// RUN: %check_clang_tidy %s llvm-use-auto %t

namespace llvm {
template <typename T, typename U>
T* dyn_cast(const U*) {
  return nullptr;
}

template <typename T, typename U>
T& cast(const U&) {
  auto ptr = new T();
  return *ptr;
}

}

namespace std {

template <typename T>
class shared_ptr {
public:
  shared_ptr() = default;
};

template< class T, class... Args >
shared_ptr<T> make_shared( Args&&... args ) {
  return shared_ptr<T>();
}

}

struct A { };
struct B: A { };

void dyn_cast_test() {
  int *kNull = nullptr;

  int *a1 = llvm::dyn_cast<int>(kNull);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: auto *a1 = llvm::dyn_cast<int>(kNull);

  int *b1, *b2 = llvm::dyn_cast<int>(kNull);

  auto *a2 = llvm::dyn_cast<int>(kNull);

  A *a3 = llvm::dyn_cast<A>(kNull);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: auto *a3 = llvm::dyn_cast<A>(kNull);

  const A *a4 = llvm::dyn_cast<A>(kNull);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: const auto *a4 = llvm::dyn_cast<A>(kNull);

  A *a5 = llvm::dyn_cast<B>(kNull);
  B *a6 = reinterpret_cast<B*>(llvm::dyn_cast<A>(kNull));

  const long long *a7 = llvm::dyn_cast<long long>(kNull);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: const auto *a7 = llvm::dyn_cast<long long>(kNull);

  const long long *const a8 = llvm::dyn_cast<long long>(kNull);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: const auto *const a8 = llvm::dyn_cast<long long>(kNull);
}


void cast() {
  int foo_int = 0;

  long long &b1 = llvm::cast<long long>(foo_int);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: auto &b1 = llvm::cast<long long>(foo_int);

  int c1 = llvm::cast<long long>(foo_int);

  long long c2 = llvm::cast<long long>(foo_int);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: auto c2 = llvm::cast<long long>(foo_int);

  const long long &b2 = llvm::cast<long long>(foo_int);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: const auto &b2 = llvm::cast<long long>(foo_int);

  const A &b3 = llvm::cast<B>(foo_int);

  const B &b4 = llvm::cast<B>(foo_int);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: const auto &b4 = llvm::cast<B>(foo_int);
}

void make_shared_test() {
  const std::shared_ptr<int> ptr = std::make_shared<int>(10);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: const auto ptr = std::make_shared<int>(10);
}

