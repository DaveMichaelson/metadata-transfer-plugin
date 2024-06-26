
int main(int argc, char **argv) {
    auto lambda = [] (int i) { return i + 5; };
    return lambda(argc);
}