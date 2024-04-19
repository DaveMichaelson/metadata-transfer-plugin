
#define ASSERT( x )  if( ( x ) == 0 ) { for( ;; ); };
#define MAKRO2 argc + a.foo() + pb->foo() + fun_pointer(&i)
#define TESTVAR int testvar = 0;

TESTVAR

class A {
public:
    virtual int foo() {
        return 5;
    }
};

class B : public A {
public:
    virtual int foo() {
        return 10;
    }
};

class C {
    A a;
};

class Base {
    int i;
};

class Sub : public Base {
    
};

class Contain {
    Base b;
};

int calc(int *a) {
    return *a + *a;
}

class FunSub : public Contain {};

int (*fun_pointer)(int *) = &calc;

int fun(Base *b) {
    return 1;
}

int fun2(Contain *c) {
    return 1;
}

int main(int argc, char **argv) {
    int (*fun_p_base)(Base *) = &fun;
    int (*fun_p_contain)(Contain *) = &fun2;
    Sub s;
    Contain contain;
    FunSub fsc;
    class FunSub : public Base{};
    FunSub fs;
    fun_p_base(&s);
    fun_p_contain(&contain);
    fun_p_base(&fs);
    fun_p_contain(&fsc);
    A a;
    B b;
    A *pb = &b;
    C c;
    testvar = argc;
    int i = 5;
    ASSERT(argc)
    // [[clang::metadata("CodeMetadata")]]
    return MAKRO2;
}