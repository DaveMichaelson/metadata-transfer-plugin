
#include <vector>

class Parent { public: int i = 1; };

class Child : public Parent {};

int foo(std::vector<Parent *> &p) {
    return p[0]->i;
}
int (*fun_pointer)(std::vector<Parent*>&) = foo;

int main(void) {
    Parent *p = new Child();
    std::vector<Parent *> vp {p};
    return fun_pointer(vp);
}