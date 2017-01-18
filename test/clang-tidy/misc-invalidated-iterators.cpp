// RUN: %check_clang_tidy %s misc-invalidated-iterators %t

// Sample std::vector implementation.
namespace std {

template <typename Type>
class vector {
  Type tmp;

public:
  class iterator {
    Type *data;

  public:
    iterator(Type *ptr) : data(ptr) {}
    Type &operator*() { return *data; }
  };

  vector() {}

  void push_back(Type &&elem) {}
  void pop_back() {}

  Type &operator[](int position) { return tmp; }

  iterator begin() { return iterator(&tmp); }
};
}

// Correct std::vector use.
void correct_sample() {
  std::vector<int> VecGood;
  VecGood.push_back(3);

  int &ElemRef = VecGood[0];
  ElemRef++;
  ElemRef *= ElemRef;
  auto ElemIter = VecGood.begin();
  (*ElemIter)++;
  int *ElemPtr = &VecGood[0];
  (*ElemPtr)++;
}

// Incorrect std::vector use.
void incorrect_sample() {
  std::vector<int> VecBad;
  VecBad.push_back(3);

  int &ElemRef = VecBad[0];
  auto ElemIter = VecBad.begin();
  int *ElemPtr = &VecBad[0];

  VecBad.push_back(42);
  ElemRef = 42;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: 'ElemRef' might be invalidated before the access
  // CHECK-MESSAGES: :[[@LINE-3]]:10: note: possible place of invalidation
  (*ElemIter)++;
  // FIXME: Add a warning here.
  (*ElemPtr)++;
  // FIXME: Add a warning here.
}


void modifyVector(std::vector<int> &Vec) {
  Vec.push_back(555);
}

void callModifyVector(std::vector<int> &Vec) {
  modifyVector(Vec);
}

void checkVector(const std::vector<int> &){}
void anotherCheckVector(std::vector<int>) {}


void test_calling() {
  std::vector<int> VecBad, VecGood;
  VecBad.push_back(5);
  VecGood.push_back(18);
  VecGood.push_back(1818);
  int &BadRef = VecBad[0];
  int &GoodRef = VecGood[0];

  modifyVector(VecBad);
  checkVector(VecGood);
  anotherCheckVector(VecGood);
  VecGood.pop_back();

  BadRef++;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: 'BadRef' might be invalidated before the access
  // CHECK-MESSAGES: :[[@LINE-7]]:3: note: possible place of invalidation
  GoodRef++;
}

void test_recursive_calling() {
  std::vector<int> RecVecBad, LambdaVecBad;
  RecVecBad.push_back(99);
  LambdaVecBad.push_back(0);
  int &BadRef = RecVecBad[0];
  int &BadLambdaRef = LambdaVecBad[0];

  auto lambda = [](std::vector<int> &Vec) {
    callModifyVector(Vec);
  };

  callModifyVector(RecVecBad);
  BadRef++;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: 'BadRef' might be invalidated before the access
  // CHECK-MESSAGES: :[[@LINE-3]]:3: note: possible place of invalidation

  lambda(LambdaVecBad);
  BadLambdaRef++;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: 'BadLambdaRef' might be invalidated before the access
  // CHECK-MESSAGES: :[[@LINE-3]]:3: note: possible place of invalidation
}

void test_argument(std::vector<int> &a, std::vector<int> &b) {
  int &ElemRef = a[0];
  int &ElemRefGood = b[0];
  a.push_back(42);
  ElemRefGood += 42;
  ElemRef += 42;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: 'ElemRef' might be invalidated before the access
  // CHECK-MESSAGES: :[[@LINE-4]]:5: note: possible place of invalidation
}