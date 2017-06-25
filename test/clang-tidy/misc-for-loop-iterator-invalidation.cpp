// RUN: %check_clang_tidy %s misc-for-loop-iterator-invalidation %t

namespace std {

template <typename First, typename Second>
class pair { };

template <typename Key, typename Value>
class unordered_map {
public:
  using value_type = pair<Key, Value>;
  using iterator = value_type*;

  Value& operator[](const Key&) { return *new Key(); }

  void erase(const Key&) { }
  void insert(const Key&, const Value&) { }

  iterator find(const Key&) const { return nullptr; }

  iterator begin() { return nullptr; }
  iterator begin() const { return nullptr; }
  iterator end() { return nullptr; }
  iterator end() const { return nullptr; }
};

template <typename Value>
class vector {
public:
  using iterator = Value*;

  Value& operator[](int) { return *new Value(); }
  const Value& operator[](int) const { return *new Value(); }

  void erase(iterator) { }

  void push_back(Value) { }

  iterator begin() { return nullptr; }
  iterator begin() const { return nullptr; }
  iterator end() { return nullptr; }
  iterator end() const { return nullptr; }
};

template <typename Value>
class list {
public:
  using iterator = Value*;

  void push_front(Value) { }

  iterator begin() { return nullptr; }
  iterator begin() const { return nullptr; }
  iterator end() { return nullptr; }
  iterator end() const { return nullptr; }
};

} // namespace std

[[ noreturn ]] void NoReturn() {
  throw 42;
}


void foo() {
  std::unordered_map<int, int> unordered_map;

  unordered_map.erase(10);

  for (auto& z : unordered_map) {
    unordered_map[1] = 42;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: this call may lead to iterator invalidation [misc-for-loop-iterator-invalidation]

    unordered_map.erase(10);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning:


    unordered_map.insert(12, 34);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning:

    unordered_map.find(123);

    unordered_map.begin();
    unordered_map.end();
  }

  unordered_map.erase(10);

  std::unordered_map<int, int> unordered_map2;

  for (auto& z : unordered_map2) {
    unordered_map[1] = 42;
    unordered_map.erase(10);

    unordered_map2.erase(10);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning:
  }

  std::vector<char> vector;
  for (auto& e: vector) {
    vector[0] = char(e);

    vector.begin();

    vector.erase(nullptr);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: this call may lead to iterator invalidation [misc-for-loop-iterator-invalidation]


    if (vector[0] == 'a') {
      vector.erase(nullptr); // OK
      break;
    }
    else if (vector[0] == 'b') {
      vector.erase(nullptr); // also OK
      return;
    }

    if (vector[1] == 'g') {
      vector.erase(nullptr); // also OK
      NoReturn();
    }
  }

  std::vector<char>& vector_reference = vector;
  for (auto& e: vector_reference) {
    vector_reference.push_back(e);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: this call may lead to iterator invalidation [misc-for-loop-iterator-invalidation]
  }

  std::list<int> list;
  for (auto& e: list) {
    list.push_front(0);
  }
}

void foo2(std::vector<int> v)  {
  for (auto x: v) {
    v.push_back(x);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: this call may lead to iterator invalidation [misc-for-loop-iterator-invalidation]
  }
}

void foo3(std::vector<int>& v)  {
  for (auto x: v) {
    v.push_back(x);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: this call may lead to iterator invalidation [misc-for-loop-iterator-invalidation]
  }
}

class vector_subclass: public std::vector<int> {

};

void foo4() {
  vector_subclass v;
  for (auto x: v) {
    v.push_back(x);
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: this call may lead to iterator invalidation [misc-for-loop-iterator-invalidation]
  }
}
