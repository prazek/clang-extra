// RUN: %check_clang_tidy %s modernize-use-clamp %t -- -- --std=c++1z

namespace std {
  template<typename C>
  C max(C x, C y) {
    return x; //whatever
  }

  template<typename C>
  C min(C x, C y) {
    return x; //whatever
  }
}

int my_clamp_max_min(int val, int min, int max) {
  return std::max(std::min(val, max), min);
}
// CHECK-MESSAGES: :[[@LINE-2]]:10: warning: use std::clamp instead of std::min and std::max
// CHECK-FIXES: return std::clamp(val, min, max);

int my_clamp_min_max(int val, int min, int max) {
  return std::min(std::max(val, min), max);
}
// CHECK-MESSAGES: :[[@LINE-2]]:10: warning: use std::clamp instead of std::min and std::max
// CHECK-FIXES: return std::clamp(val, min, max);

int my_clamp_max_min2(int val, int min, int max) {
  return std::max(min, std::min(val, max));
}
// CHECK-MESSAGES: :[[@LINE-2]]:10: warning: use std::clamp instead of std::min and std::max
// CHECK-FIXES: return std::clamp(val, min, max);

int my_clamp_min_max2(int val, int min, int max) {
  return std::min(max, std::max(val, min));
}
// CHECK-MESSAGES: :[[@LINE-2]]:10: warning: use std::clamp instead of std::min and std::max
// CHECK-FIXES: return std::clamp(val, min, max);


// Argument ambiguity examples - it's equivalent, but may be confusing
int my_clamp_max_min_reversed_args(int val, int min, int max) {
  return std::max(std::min(max, val), min);
}
// CHECK-MESSAGES: :[[@LINE-2]]:10: warning: use std::clamp instead of std::min and std::max
// CHECK-FIXES: std::clamp(max, min, val);

int my_clamp_min_max_reversed_args(int val, int min, int max) {
  return std::min(std::max(min, val), max);
}
// CHECK-MESSAGES: :[[@LINE-2]]:10: warning: use std::clamp instead of std::min and std::max
// CHECK-FIXES: std::clamp(min, val, max);

// Controversial example
int double_min(int val, int min, int max) {
  return std::max(std::min(val, max), std::min(val, max));
}
// CHECK-MESSAGES: :[[@LINE-2]]:10: warning: use std::clamp instead of std::min and std::max
// CHECK-FIXES: std::clamp(val, std::min(val, max), max);

// Negative examples:
void no_clamp() {
  int val, min, max;
  std::max(val, min);
  std::min(val, max);
  std::min(val, std::min(val, val));
  std::max(val, std::max(val, val));
}
