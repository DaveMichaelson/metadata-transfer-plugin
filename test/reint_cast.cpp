
int main(int argc, char **argv) {
    int* a = new int();
    void* b = reinterpret_cast<void*>(a);
    int* c = reinterpret_cast<int*>(b);
}