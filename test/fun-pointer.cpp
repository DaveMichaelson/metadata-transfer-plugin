
int calc(int *a) {
    return *a + *a;
}

int (*fun_pointer)(int *) = &calc;

int main(void) {
    int i = 5;
    return fun_pointer(&i);
}