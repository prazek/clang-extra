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

void some_foo() {
  int *z = nullptr;
  int *a1 = llvm::dyn_cast<int>(z);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: auto *a1 = llvm::dyn_cast<int>(z);

  auto *a2 = llvm::dyn_cast<int>(z);

  const long long *b1 = llvm::dyn_cast<long long>(z);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: const auto *b1 = llvm::dyn_cast<long long>(z);

  int foo_int = 0;

  long long &b2 = llvm::cast<long long>(foo_int);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: auto &b2 = llvm::cast<long long>(foo_int);

  long long &b3 = llvm::cast<long long>(foo_int);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type; use auto instead [llvm-use-auto]
  // CHECK-FIXES: auto &b3 = llvm::cast<long long>(foo_int);
}

