
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

int calc(int *a) {
    return *a + *a;
}

int (*fun_pointer)(int *) = &calc;

int main(int argc, char **argv) {
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