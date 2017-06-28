// RUN: %check_clang_tidy %s misc-bool-to-integer-conversion %t

const int is42Answer = true;
// CHECK-MESSAGES: :[[@LINE-1]]:24: warning: implicitly converting bool literal to 'int'; use integer literal instead [misc-bool-to-integer-conversion]
// CHECK-FIXES: const int is42Answer = 1;{{$}}

volatile int noItsNot = false;
// CHECK-MESSAGES: :[[@LINE-1]]:25: warning: implicitly converting bool literal to 'int'; {{..}}
// CHECK-FIXES: volatile int noItsNot = 0;{{$}}
int a = 42;
int az = a;

long long ll = true;
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: implicitly converting bool literal to 'long long';{{..}}
// CHECK-FIXES: long long ll = 1;{{$}}

void fun(int) {}
#define ONE true

// No fixup for macros.
int one = ONE;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: implicitly converting bool literal to 'int'; use integer literal instead [misc-bool-to-integer-conversion]

void test() {
  fun(ONE);
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: implicitly converting bool{{..}}

  fun(42);
  fun(true);
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: implicitly {{..}}
// CHECK-FIXES: fun(1);{{$}}
}

char c = true;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: implicitly {{..}}
// CHECK-FIXES: char c = 1;

float f = false;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: implicitly converting bool literal to 'float';{{..}}
// CHECK-FIXES: float f = 0;

struct Blah {
  Blah(int blah) { }
};

const int &ref = false;
// CHECK-MESSAGES: :[[@LINE-1]]:18: warning: implicitly converting bool literal to 'int'{{..}}
// CHECK-FIXES: const int &ref = 0;

Blah bla = true;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: implicitly converting bool literal to 'int'{{..}}
// CHECK-FIXES: Blah bla = 1;

Blah bla2 = 1;

char c2 = 1;
char c3 = '0';
bool b = true;

// Don't warn of bitfields of size 1. Unfortunately we can't just
// change type of flag to bool, because some compilers like MSVC doesn't
// pack bitfields of different types.
struct BitFields {
  BitFields() : a(true), flag(false) {}
// CHECK-MESSAGES: :[[@LINE-1]]:19: warning: implicitly converting
// CHECK-FIXES: BitFields() : a(1), flag(false) {}

  unsigned a : 3;
  unsigned flag : 1;
};

struct some_struct {
  bool p;
  int z;
};

void testBitFields() {
  BitFields b;
  b.flag = true;
  b.a = true;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: implicitly converting
// CHECK-FIXES: b.a = 1;

  b.flag |= true;
  some_struct s;
  s.p |= true;
  s.z |= true;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: implicitly converting
// CHECK-FIXES: s.z |= 1;
}

bool returnsBool() { return true; }

void test_operators() {
  bool p = false;
  if (p == false) {

  }
  int z = 1;
  if (z == true) {
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: implicitly converting
// CHECK-FIXES: if (z == 1) {

  }
  bool p2 = false != p;

  int z2 = z - true;
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: implicitly converting
// CHECK-FIXES: int z2 = z - 1;

  bool p3 = z + true;
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: implicitly converting
// CHECK-FIXES: bool p3 = z + 1;

  if(true && p == false) {}
  if (returnsBool() == false) {}
}

bool ok() {
  return true;
  {
    return false;
  }
  bool z;
  return z;
}


int return_int() {
  return 2;
  {
    return true;
    // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: implicitly converting bool literal
    // CHECK-FIXES: return 1;
  }
  bool p;
  return p;
}

int return_int2() {
  {
    return true;
    // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: implicitly converting bool literal
    // CHECK-FIXES: return 1;
  }
  bool p;
  return p;

  int z;
  return z;
}

// This checks if it doesn't match inner lambda returns
void yat() {
  auto l = [](int p) {
    return p == 42;
  };
}


struct A {
  virtual int fun() = 0;
  int fun2() { return 5; };
};


