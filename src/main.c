#include <stdio.h>
#include <getopt.h>
#include <ctype.h> // isprint()
#include <stdlib.h>
#include "util.h"
#include "parser.h"
#include "scanner.h"
#include "ast.h"
#include "safe.h"

#ifdef _WIN32
#include <windows.h>

HANDLE handle;
CONSOLE_SCREEN_BUFFER_INFO console;
WORD saved_attributes;
#endif

#define DEFAULT_OUTPUT "a.out.c"

int
main(int argc, char *argv[]) {
    int opt, status = 0, file_count;
    char *out_filename = NULL;
    yyscan_t scanner;
    YY_BUFFER_STATE state;
    FILE *output;

    #ifdef _WIN32
    handle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(handle, &console);
    saved_attributes = console.wAttributes;
    #endif

    opterr = 0;
    while (-1 != (opt = getopt(argc, argv, ":o:"))) {
        switch (opt) {
            case 'o':
                out_filename = safe_strdup(optarg);
                break;
            case ':':
                print_error("option '-%c' requires an argument.\n", optopt);
                status = 1;
                break;
            case '?':
                if (isprint(optopt)) {
                    print_error("unrecognized command line option '-%c'.\n",
                        optopt);
                } else {
                    print_error("unknown option character '\\x%x'.\n", optopt);
                }
                status = 1;
                break;
            default:
                status = 1;
                break;
        }
    }
    file_count = argc - optind;
    if (0 == file_count) {
        print_error("no input files.\n");
        status = 1;
    }
    FILE *inputs[file_count];
    for (int i = 0; i < file_count; i++) {
        if (NULL == (inputs[i] = fopen(argv[optind + i], "r"))) {
            print_error("%s: ", argv[optind + i]);
            perror("");
            status = 1;
        }
    }
    if (NULL != out_filename) {
        if (NULL == (output = fopen(out_filename, "w"))) {
            print_error("%s: ", out_filename);
            perror("");
            status = 1;
        }
    } else {
        if (NULL == (output = fopen(DEFAULT_OUTPUT, "w"))) {
            print_error("%s: ", DEFAULT_OUTPUT);
            perror("");
            status = 1;
        }
    }
    if (status) {
        exit(EXIT_FAILURE);
    }
    if (yylex_init(&scanner)) {
        print_ICE("could not initialize Flex scanner.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < file_count; i++) {
        state = yy_create_buffer(inputs[i], YY_BUF_SIZE, scanner);
        yy_switch_to_buffer(state, scanner);
        AST *root = NULL;
        if (yyparse(&root, argv[optind + i], scanner)) {
            status = 1;
        } else {
            //Type checking, code generation, etc...
            //json_AST(root, stdout, 0);
            //fprintf(stdout, "\n");
            if (TypeCheck(root)) {
                print_error("type checker failed\n");
            } else {
                CodeGen(root, output);
            }
            delete_AST(root);
        }
        yy_delete_buffer(state, scanner);
    }
    yylex_destroy(scanner);
    for (int i = 0; i < file_count; i++) {
        fclose(inputs[i]);
    }
    free(out_filename);
    return 0;
}
