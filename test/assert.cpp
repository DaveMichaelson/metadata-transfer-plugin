#define ASSERT( x )  if( ( x ) == 0 ) { for( ;; ); };

int main(int argc, char **argv) {
    ASSERT(argc)
    return argc;
}