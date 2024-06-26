#define SUB_MACRO i
#define EXPR_MACRO argc + SUB_MACRO
#define ASSIGN_MACRO argc = EXPR_MACRO;
#define STMT_MACRO return EXPR_MACRO;

int main(int argc, char **argv) {
    int i = 5;
    ASSIGN_MACRO
    STMT_MACRO
}