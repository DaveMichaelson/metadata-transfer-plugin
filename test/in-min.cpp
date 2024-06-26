class A {
public:
    int i;
    virtual int foo(int* d) {return *d + i;};
};

class B : public A {};

class C {
public:
    A a;
    virtual int bar(int* d) {return *d + a.i;};
};

int main(int argc, char** args) {
    B b;
    C c;
    return argc + b.i + c.a.i;
}
