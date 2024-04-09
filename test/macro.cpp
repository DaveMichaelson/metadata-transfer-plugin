
#define ASSERT( x )  if( ( x ) == 0 ) { for( ;; ); };
#define MAKRO2 return argc;
#define TESTVAR int testvar = 0;

TESTVAR

int main(int argc, char **argv) {
    testvar = argc;
    ASSERT(argc)
    MAKRO2
}