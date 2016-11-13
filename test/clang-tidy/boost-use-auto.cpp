// RUN: %check_clang_tidy %s boost-use-auto %t

namespace llvm {
template <typename T, typename U>
T* dyn_cast(const U*) {
  return nullptr;
}
}

void some_foo() {
  int *z = nullptr;
  int *a1 = llvm::dyn_cast<int>(z);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast; use auto instead [boost-use-auto]
  // CHECK-FIXES: auto a1 = llvm::dyn_cast<int>("42");

  auto *a2 = llvm::dyn_cast<int>(z);

  const long long *b1 = llvm::dyn_cast<long long>(z);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: redundant type for lexical_cast; use auto instead [boost-use-auto]
  // CHECK-FIXES: const auto b1 = llvm::dyn_cast<long long>("123");

  long long *b2 = llvm::dyn_cast<long long>(z);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast; use auto instead [boost-use-auto]
  // CHECK-FIXES: auto b2 = llvm::dyn_cast<long long>(42);
}
