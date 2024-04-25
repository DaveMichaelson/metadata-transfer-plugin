#define ASSERT( x )  if( ( x ) == 0 ) { for( ;; ); };
#define TESTVAR int testvar = 0;
#define SUB_MACRO testvar
#define EXPR_MACRO argc + SUB_MACRO

TESTVAR

int main(int argc, char **argv) {
    ASSERT(argc)
    return EXPR_MACRO;
}