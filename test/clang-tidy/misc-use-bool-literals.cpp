// RUN: %check_clang_tidy %s misc-use-bool-literals %t

bool bar1 = 1;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: implicitly converting integer literal to bool, use bool literal instead [misc-use-bool-literals]
// CHECK-FIXES: {{^}}bool bar1 = true;{{$}}

bool bar2 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: {{.*}}
// CHECK-FIXES: {{^}}bool bar2 = false;{{$}}

bool bar3 = 0x123ABCLL;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: {{.*}}
// CHECK-FIXES: {{^}}bool bar3 = true;{{$}}

#define TRUE_FALSE 1

bool bar4 = TRUE_FALSE;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: implicitly converting integer literal to bool [misc-use-bool-literals]

#define TRUE_FALSE2 bool(1) // OK

bool bar6 = true; // OK

void foo4(bool bar) {

}

char bar7 = 0;
unsigned long long bar8 = 1;

int main() {
  foo4(1);
  // CHECK-MESSAGES: :[[@LINE-1]]:8: warning: {{.*}}
  // CHECK-FIXES: {{^}}  foo4(true);{{$}}

  foo4(false); // OK

  bar1 = 0;
  // CHECK-MESSAGES: :[[@LINE-1]]:10: warning: {{.*}}
  // CHECK-FIXES: {{^}}  bar1 = false;{{$}}
}
