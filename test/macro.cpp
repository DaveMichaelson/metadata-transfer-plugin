
#define ASSERT( x )  if( ( x ) == 0 ) { for( ;; ); };

int testvar = 0;

int main(int argc, char **argv) {
    testvar = argc;
    ASSERT(argc)
    return argc;
}