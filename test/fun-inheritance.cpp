class Parent1 { public: virtual int foo(int *i) {return *i;} };
class Child1a : public Parent1 { public: virtual int foo(int *i) {return (*i) + 0;} };
class Child1b : public Parent1 { public: virtual int foo(int *i) {return (*i) + 1;} };
class Child1c : public Parent1 { public: virtual int foo(int *i) {return (*i) + 2;} };
class Composition1 { Parent1 p; public: int cFoo(int *i) {return p.foo(i);} };

class Parent2 { Parent1 parent1; public: virtual int bar(int *i) {return *i;} };
class Child2a : public Parent2 { public: virtual int bar(int *i) {return (*i) + 0;} };
class Child2b : public Parent2 { public: virtual int bar(int *i) {return (*i) + 1;} };
class Child2c : public Parent2 { public: virtual int bar(int *i) {return (*i) + 2;} };
class Composition2 { Parent2 p; public: int cBar(int *i) {return p.bar(i);} };

struct WrapperP {
    virtual Parent1* get_pointer() {
        return nullptr;
    }
};

class WrapperC : public WrapperP {
    private:
    Parent1* obj;

    public:
    WrapperC(Parent1* obj) : obj(obj) {}
    virtual Parent1* get_pointer() override {
        return obj;
    }
};

struct WrapperP2 {
    virtual Parent2* get_pointer() {
        return nullptr;
    }
};

class WrapperC2 : public WrapperP2 {
    private:
    Parent2* obj;

    public:
    WrapperC2(Parent2* obj) : obj(obj) {}
    virtual Parent2* get_pointer() override {
        return obj;
    }
};

// int (*fun)(void *, int *);

int dosth(WrapperP* wrapper) {
    Parent1* p = wrapper->get_pointer();
    if (p != nullptr) {
        int i = 5;
        return p->foo(&i);
    }
    return 0;
}

int dosth2(WrapperP2* wrapper) {
    Parent2* p = wrapper->get_pointer();
    if (p != nullptr) {
        int i = 5;
        return p->bar(&i);
    }
    return 0;
}

int main(int argc, char **argv) {
    Child1a ca;
    Child2a c2a;
    int i = 0;

    WrapperP* wrap = new WrapperC(&ca);
    // dosth(wrap);
    WrapperP2* wrap2 = new WrapperC2(&c2a);

    return dosth(wrap) + dosth2(wrap2);
}
