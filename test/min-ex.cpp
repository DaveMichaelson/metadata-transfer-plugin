int i = 1;
class A {public: int a = 2;};

int main(int argc, char **argv) {
    A a;
    return argc + i + a.a;
}