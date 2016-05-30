// RUN: %check_clang_tidy %s modernize-increment-bool %t

#define INCR(var) ((var)++)

template <typename T>
T &f(T &x) { return x; }

template <typename T>
void g() {
  T x;
  x++;

  bool y;
  y++;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: bool incrementation is deprecated, use assignment
  // CHECK-FIXES: y = true;
}

int main() {
  bool b1 = false, b2 = false, b3 = false;

  b1++;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: bool incrementation is deprecated, use assignment
  // CHECK-FIXES: b1 = true;

  bool b4 = f(b1)++;
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: bool incrementation is deprecated, use assignment

  bool b5 = ++f(b1);
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: bool incrementation is deprecated, use assignment
  // CHECK-FIXES: bool b5 = (f(b1) = true);

  (b1 = false)++;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: bool incrementation is deprecated, use assignment
  // CHECK-FIXES: (b1 = false) = true;

  (b1 = b2 = false)++;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: bool incrementation is deprecated, use assignment
  // CHECK-FIXES: (b1 = b2 = false) = true;

  ++(b1 = b2 = false);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: bool incrementation is deprecated, use assignment
  // CHECK-FIXES: (b1 = b2 = false) = true;

  b3 = b1++ && b2;
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: bool incrementation is deprecated, use assignment

  b3 = ++(b1 |= b4) && b3;
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: bool incrementation is deprecated, use assignment
  // CHECK-FIXES: b3 = ((b1 |= b4) = true) && b3;

  int x = 8;
  x++;

  INCR(b1);

  g<int>();
  g<bool>();
}
