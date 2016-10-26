// RUN: %check_clang_tidy %s boost-lexical-cast-redundant-type %t

namespace boost {
template <typename T, typename U>
T lexical_cast(const U &) { return T(); }
}

void same_type() {
  // FIXME: Add something that triggers the check here.
  int a1 = boost::lexical_cast<int>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: redundant type for lexical_cast. Use auto instead [boost-lexical-cast-redundant-type]
  // CHECK-FIXES: auto a1 = boost::lexical_cast<int>("42");

  long long a2 = boost::lexical_cast<long long>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: redundant type for lexical_cast;
  // CHECK-FIXES: auto a2 = boost::lexical_cast<long long>("42");

  using MyType = unsigned long long;
  MyType a3 = boost::lexical_cast<MyType>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: redundant type for lexical_cast;
  // CHECK-FIXES: auto a3 = boost::lexical_cast<MyType>("42");

  unsigned long long a4 = boost::lexical_cast<MyType>("2137");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: redundant type for lexical_cast;
  // CHECK-FIXES: auto a4 = boost::lexical_cast<MyType>("2137");

  // multiple decls
  int a9 = boost::lexical_cast<int>("42"), a10 = boost::lexical_cast<int>("24");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: redundant type for lexical_cast;
  // CHECK-FIXES: a9 = boost::lexical_cast<int>("42"), a10 = boost::lexical_cast<int>("24");


  MyType a11 = boost::lexical_cast<MyType>("42"), a12 = boost::lexical_cast<unsigned long long>("24");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: redundant type for lexical_cast;
  // CHECK-FIXES: auto a11 = boost::lexical_cast<MyType>("42"), a12 = boost::lexical_cast<unsigned long long>("24");

  unsigned long long a13 = boost::lexical_cast<unsigned long long>("42"), a14 = boost::lexical_cast<MyType>("24");
  // CHECK-FIXES: auto a13 = boost::lexical_cast<unsigned long long>("42"), a14 = boost::lexical_cast<MyType>("24");

  a13 = boost::lexical_cast<unsigned long long>("42");

  int a15 = boost::lexical_cast<int>("42"), a16 = 15;
  // CHECK-FIXES: auto a13 = boost::lexical_cast<unsigned long long>("42"), a14 = boost::lexical_cast<MyType>("24");

  long long *p = boost::lexical_cast<long long*>("fdas");

}

void different_type() {
  int a5 = boost::lexical_cast<long long>("1488");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: different types for lexical_cast;
  // CHECK-FIXES: auto a5 = boost::lexical_cast<long long>("1488");

  long long a6 = boost::lexical_cast<int>("2137");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: different types for lexical_cast;
  // CHECK-FIXES: auto a6 = boost::lexical_cast<int>("2137");

  using MyType = unsigned long long;
  long long a7 = boost::lexical_cast<MyType>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: different types for lexical_cast;
  // CHECK-FIXES: auto a7 = boost::lexical_cast<MyType>("42");

  MyType a8 = boost::lexical_cast<int>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: different types for lexical_cast;
  // CHECK-FIXES: auto a8 = boost::lexical_cast<int>("42");

  // multiple decls with different types - no fixit
  int a9 = boost::lexical_cast<long long>("42"), a10 = boost::lexical_cast<int>("24");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: different types for lexical_cast;

}

// This should not generate any warnings.
void no_warnings(int x) {

  auto a4 = boost::lexical_cast<unsigned>("-42");
  auto a5 = boost::lexical_cast<unsigned>(x);
  auto a6 = boost::lexical_cast<int>("42"), a7 = boost::lexical_cast<int>("24");

  boost::lexical_cast<int>("32");
  no_warnings(boost::lexical_cast<unsigned>(32));
}

// FIXME: Verify the applied fix.
//   * Make the CHECK patterns specific enough and try to make verified lines
//     unique to avoid incorrect matches.

// FIXME: Add something that doesn't trigger the check here.