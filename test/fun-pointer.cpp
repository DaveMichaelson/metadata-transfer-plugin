
int calc(int const*const a) {
    return *a + *a;
}

int (*fun_pointer)(int const*const) = &calc;

int main(void) {
    int i = 5;
    return fun_pointer(&i);
}