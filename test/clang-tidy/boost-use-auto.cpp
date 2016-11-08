// RUN: %check_clang_tidy %s boost-use-auto %t


namespace boost {
template <typename T, typename U>
T lexical_cast(const U &) { return T(); }
}

void same_type() {
  int a1 = boost::lexical_cast<int>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast; use auto instead [boost-use-auto]
  // CHECK-FIXES: auto a1 = boost::lexical_cast<int>("42");

  auto at = boost::lexical_cast<int>("42");

  long long a2 = boost::lexical_cast<long long>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast;
  // CHECK-FIXES: auto a2 = boost::lexical_cast<long long>("42");

  using MyType = unsigned long long;
  MyType a3 = boost::lexical_cast<MyType>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast;
  // CHECK-FIXES: auto a3 = boost::lexical_cast<MyType>("42");

  unsigned long long a4 = boost::lexical_cast<MyType>("2137");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast;
  // CHECK-FIXES: auto a4 = boost::lexical_cast<MyType>("2137");

  // multiple decls
  int a9 = boost::lexical_cast<int>("42"), a10 = boost::lexical_cast<int>("24");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast;
  // CHECK-FIXES: a9 = boost::lexical_cast<int>("42"), a10 = boost::lexical_cast<int>("24");


  MyType a11 = boost::lexical_cast<MyType>("42"), a12 = boost::lexical_cast<unsigned long long>("24");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast;
  // CHECK-FIXES: auto a11 = boost::lexical_cast<MyType>("42"), a12 = boost::lexical_cast<unsigned long long>("24");

  unsigned long long a13 = boost::lexical_cast<unsigned long long>("42");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: redundant type for lexical_cast;
  // CHECK-FIXES: auto a13 = boost::lexical_cast<unsigned long long>("42");

  a13 = boost::lexical_cast<unsigned long long>("42");

}