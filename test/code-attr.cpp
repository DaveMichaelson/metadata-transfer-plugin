
int main(int argc, char **argv) {
    int i = 5;
    [[clang::metadata("CodeMetadata")]]
    return argc + i;
}