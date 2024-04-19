
class Base {
    int i;
};

class Sub : public Base {};

class Composition {
    Base b;
};

int fun_base(Base *b) {
    return 1;
}

int fun_composition(Composition *c) {
    return 1;
}

int main(int argc, char **argv) {
    int (*fun_p_base)(Base *) = &fun_base;
    int (*fun_p_composition)(Composition *c) = &fun_composition;

    Sub s;
    Composition c;

    class Sub : public Base {};
    class Composition {Base b;};

    Sub funS;
    Composition funC;

    fun_p_base(&s);
    fun_p_base(&funS);
    fun_p_composition(&c);

    return 0;
}