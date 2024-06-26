
#include <vector>

class Parent { public: int i = 1; };

class Child : public Parent {};

int foo(const std::vector<const Parent *> &p) {
    return p[0]->i;
}
int (*fun_pointer)(const std::vector<const Parent*>&) = foo;

int main(void) {
    Parent *p = new Child();
    std::vector<const Parent *> vp {p};
    return fun_pointer(vp);
}