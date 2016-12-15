// RUN: %check_clang_tidy %s obvious-invalid-range %t

namespace std {

template <class InputIterator, class OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result) {
  return result;
}

template <class InputIterator, class OutputIterator>
OutputIterator move(InputIterator first, InputIterator last, OutputIterator result) {
  return result;
}

template <typename T>
T move(T &&) { return T(); }

template <class InputIterator, typename Val>
void fill(InputIterator first, InputIterator last, const Val &v) {
}

template <typename T>
class vector {
public:
  T *begin();
  T *end();
};
}

void test_copy() {
  std::vector<int> v, v2;
  std::copy(v.begin(), v2.end(), v2.begin());
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: call to algorithm with begin and end from different objects [obvious-invalid-range]

  std::copy(v.begin(), v.end(), v2.begin());
}

void test_move() {
  std::vector<int> v;
  auto &v2 = v;
  std::move(v.begin(), v2.end(), v2.begin());
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: call to algorithm with begin and end from different objects

  std::move(v.begin(), v.end(), v2.begin());
  std::move(v.begin());
  test_copy();
}
