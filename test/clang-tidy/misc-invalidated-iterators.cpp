// RUN: %check_clang_tidy %s misc-invalidated-iterators %t


// Sample std::vector implementation.
namespace std {

template<typename Type>
class vector {
  Type tmp;

public:
  class iterator {
    Type *data;

  public:
    iterator(Type *ptr) : data(ptr) {}
    Type &operator*() { return *data; }
  };

  vector() { }

  void push_back(Type &&elem) { }

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
  ElemRef++;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: 'ElemRef' might be invalidated before the access
  // CHECK-MESSAGES: :[[@LINE-3]]:10: note: possible place of invalidation
  (*ElemIter)++;
  // FIXME: Add a warning here.
  (*ElemPtr)++;
  // FIXME: Add a warning here.
}
