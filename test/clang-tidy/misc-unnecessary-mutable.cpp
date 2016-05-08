// RUN: %check_clang_tidy %s misc-unnecessary-mutable %t

struct NothingMutable {
  int field1;
  unsigned field2;
  const int field3;
  volatile float field4;

  NothingMutable(int a1, unsigned a2, int a3, float a4) :
    field1(a1), field2(a2), field3(a3), field4(a4) {}

  void doSomething() {
    field1 = 1;
    field2 = 2;
    field4 = 4;
  }
};


struct NoMethods {
  int field1;
  mutable unsigned field2;  // These cannot be fixed; they're public
  const int field3;
  mutable volatile NothingMutable field4;
};


class NoMethodsClass {
public:
  int field1;
  mutable unsigned field2;

private:
  const int field3;
  mutable volatile NothingMutable field4;
  // CHECK-MESSAGES: :[[@LINE-1]]:35: warning: 'mutable' modifier is unnecessary for field 'field4' [misc-unnecessary-mutable]
  // CHECK-FIXES: {{^  }}volatile NothingMutable field4;
};


struct PrivateInStruct {
private:
  mutable volatile unsigned long long blah;
  // CHECK-MESSAGES: :[[@LINE-1]]:39: warning: 'mutable' modifier is unnecessary for field 'blah' {{..}}
  // CHECK-FIXES: {{^  }}volatile unsigned long long blah;
};


union PrivateInUnion {
public:
  int someField;

private:
  mutable char otherField;
};


class UnusedVar {
private:
  mutable int x __attribute__((unused));
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: 'mutable' modifier is unnecessary for field 'x' {{..}}
  // CHECK-FIXES: {{^  }}int x __attribute__((unused));
};


class NoConstMethodsClass {
public:
  int field1;
  mutable unsigned field2;
  
  NoConstMethodsClass() : field2(42), field3(9), field4(NothingMutable(1, 2, 3, 4)) {}

  void doSomething() {
    field2 = 8;
    field1 = 99;
    field4.doSomething();
  }

private:
  const int field3;
  mutable NothingMutable field4;
  // CHECK-MESSAGES: :[[@LINE-1]]:26: warning: 'mutable' modifier is unnecessary for field 'field4' {{..}}
  // CHECK-FIXES: {{^  }}NothingMutable field4;
};


class ConstMethods {
private:
  mutable int field1, field2;
  // CHECK-MESSAGES: :[[@LINE-1]]:23: warning: 'mutable' modifier is unnecessary for field 'field2' {{..}}
  mutable int incr, decr, set, mul, constArg1, constArg2, constRef, ref1, ref2;
  // CHECK-MESSAGES: :[[@LINE-1]]:37: warning: 'mutable' modifier is unnecessary for field 'constArg1' {{..}}
  // CHECK-MESSAGES: :[[@LINE-2]]:48: warning: 'mutable' modifier is unnecessary for field 'constArg2' {{..}}
  // CHECK-MESSAGES: :[[@LINE-3]]:59: warning: 'mutable' modifier is unnecessary for field 'constRef' {{..}}

  void takeArg(int x) const { x *= 4; }
  int takeConstRef(const int& x) const { return x + 99; }
  void takeRef(int&) const {}

  template<typename... Args> void takeArgs(Args... args) const {}
  template<typename... Args> void takeArgRefs(Args&... args) const {}

public:
  void doSomething() const {
    field1 = field2;
  }

  void doOtherThing() const {
    incr++;
    decr--;
    set = 42;
    mul *= 3;
    takeArg(constArg1);
    takeConstRef(constRef);
    takeRef(ref1);
    takeArgs(constArg1, constArg2);
    takeArgRefs(ref1, ref2);
  }
};


class NonFinalClass {
public:
  mutable int fPublic;

protected:
  mutable int fProtected;

private:
  mutable int fPrivate;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: 'mutable' modifier is unnecessary for field 'fPrivate' {{..}}
  // CHECK-FIXES: {{^  }}int fPrivate;
};


class FinalClass final {
public:
  mutable int fPublic;

protected:
  mutable int fProtected;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: 'mutable' modifier is unnecessary for field 'fProtected' {{..}}
  // CHECK-FIXES: {{^  }}int fProtected;

private:
  mutable int fPrivate;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: 'mutable' modifier is unnecessary for field 'fPrivate' {{..}}
  // CHECK-FIXES: {{^  }}int fPrivate;
};


class NotAllFuncsKnown {
  void doSomething();
  void doSomethingConst() const {}

private:
  mutable int field;
  // Can't be fixed. We don't know if doSomething() doesn't declare a *const* NotAllFuncsKnown instance
  // and then modify 'field' field.
};


class NotAllConstFuncsKnown {
  void doSomething() {}
  void doSomethingConst() const;
  void doOtherConst() const {}

private:
  mutable int field;
};


class ConstFuncOutside {
  void doSomething();
  void doSomethingConst() const;
  void doOtherConst() const {}

private:
  mutable int field;
  // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: 'mutable' modifier is unnecessary for field 'field' {{..}}
  // CHECK-FIXES: {{^  }}int field;
};

void ConstFuncOutside::doSomethingConst() const {}


// We can't really see if mutable is necessary or not.
template<typename T>
class TemplatedClass {
public:

  void doSomething() {
    a = b = c;
  }

  void doSomethingConst() const {
    a = b = c;
  }

private:
  mutable int a, b, c;
};

TemplatedClass<int> TemplatedClassInt;


class ClassWithTemplates {
public:
  void doSomethingConst() const {
    a = b;
  }

  // Here, we can see that.
  template<typename T>
  void doOtherConst() const {
    b = c;
  }

private:
  mutable int a, b, c, d;
  // CHECK-MESSAGES: :[[@LINE-1]]:21: warning: 'mutable' modifier is unnecessary for field 'c' {{..}}
  // CHECK-MESSAGES: :[[@LINE-2]]:24: warning: 'mutable' modifier is unnecessary for field 'd' {{..}}
};


class ImplicitCast {
public:

  void doSomethingConst() const {
    a = b;
  }

private:
  mutable int a;
  mutable double b;
  // CHECK-MESSAGES: :[[@LINE-1]]:18: warning: 'mutable' modifier is unnecessary for field 'b' {{..}}
  // CHECK_FIXES: {{^  }}double b;
};

struct MutableNotFirst {
private:
    long mutable long abc = 42;
  // CHECK-MESSAGES: :[[@LINE-1]]:23: warning: 'mutable' modifier is unnecessary for field 'abc' {{..}}
  // CHECK_FIXES: {{^  }}long long abc;
    long long mutable bca;
  // CHECK-MESSAGES: :[[@LINE-1]]:23: warning: 'mutable' modifier is unnecessary for field 'bca' {{..}}
  // CHECK_FIXES: {{^  }}long long bca;
    int mutable ca;
  // CHECK-MESSAGES: :[[@LINE-1]]:17: warning: 'mutable' modifier is unnecessary for field 'ca' {{..}}
  // CHECK_FIXES: {{^  }}int ca;
};


// Fails for now.
/*
void change(const int &a) {
    const_cast<int&>(a) = 42;
}

struct Change {
    void changeMember() const {
        change(m);
        const int& x = otherM;
        const_cast<int&>(x) = 33;

        const_cast<int&>(anotherM) = -4;
    }
private:
    mutable int m;        
    mutable int otherM;
    mutable int anotherM;
};
*/


class StrangeClass {
public:
  void foo() { }
};


void EvilFunction(int& a) { }
void EvilFunction(const int& a) { }

class JustClass {
public:
  JustClass() { }

  void foo() {
    MutableClass.foo();
  }

  void foo() const;
  void evilCaller() const;

private:
  mutable StrangeClass MutableClass; // Must stay mutable (because of foo func below)
  mutable int MutableInt; // Must stay mutable
};

void JustClass::foo() const {
  MutableClass.foo();
}

void JustClass::evilCaller() const {
  EvilFunction(MutableInt);
}



class AnotherStrangeClass {
public:
	// Example of non-const method which requires that MutableInt should stay mutable.
	void strangeFoo() {
		const AnotherStrangeClass* ConstThis = this;
		EvilFunction(ConstThis->MutableInt);
	}

private:
	mutable int MutableInt; // must stay mutable
};


class ClassWithStrangeConstructor {
public:
	ClassWithStrangeConstructor() {
		const ClassWithStrangeConstructor* ConstThis = this;
		EvilFunction(ConstThis->MutableInt);
	}

private:
	mutable int MutableInt; // must stay mutable
};

class ClassWithStrangeDestructor {
public:
	~ClassWithStrangeDestructor() {
		const ClassWithStrangeDestructor* ConstThis = this;
		EvilFunction(ConstThis->MutableInt);
	}

private:
	mutable int MutableInt; // must stay mutable
};


// Don't touch friends or they'll hurt you.
class ClassWithFriends {
public:
	friend void someFriend(ClassWithFriends*);

private:
	mutable int MutableInt; // must stay mutable
};

void someFriend(ClassWithFriends* Class) {
	const ClassWithFriends* ConstClass = Class;
	EvilFunction(ConstClass->MutableInt);
}

class ClassWithClassFriends {
public:
	friend class ClassFriend;

private:
	mutable int MutableInt; // must stay mutable
};

class ClassFriend {
public:
	static void foo(const ClassWithClassFriends* Class) {
		EvilFunction(Class->MutableInt);
	}
};


