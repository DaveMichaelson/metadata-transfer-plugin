#define ASSERT( x )  if( ( x ) == 0 ) { for( ;; ); };
#define TESTVAR int testvar = 0;
#define EXPR_MACRO argc + testvar

TESTVAR

int main(int argc, char **argv) {
    ASSERT(argc)
    return EXPR_MACRO;
}