// RUN: %check_clang_tidy %s readability-for-should-be-while %t

void f() {
  int x;
  for (;;) {
    ++x;
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: this for loop should be a while loop
  // CHECK-FIXES: while (true) {

  for (; true; ) {
    ++x;
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: this for loop should be a while loop
  // CHECK-FIXES: while (true)

  for (; 1 == 2; ) {
    ++x;
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: this for loop should be a while loop
  // CHECK-FIXES: while (1 == 2)

  for (int x; ;) {

  }

  for (; ; ++x) {

  }
}
