#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/error.h"
#include "../include/codegen.h"
#include "../include/symtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char* buf = malloc(len+1);
    if (!buf) { fprintf(stderr, "out of memory\n"); exit(1); }

    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "C8L Compiler — CHIP-8 Language\n"
        "\n"
        "Usage:\n"
        "  %s <source.c8l> [options]\n"
        "\n"
        "Options:\n"
        "  -o <output.rom>   Output ROM filename (default: out.rom)\n"
        "  --dump-tokens     Print token stream and exit\n"
        "  --dump-ast        Print AST and exit\n"
        "  --dump-symbols    Print symbol table after Pass 1\n"
        "  --dump-rom        Print ROM hex dump to stdout\n"
        "  -h, --help        Show this help\n",
        prog);
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) usage(argv[0]);

    const char *input_file  = NULL;
    const char *output_file = "out.rom";
    int dump_tokens = 0;
    int dump_ast = 0;
    int dump_symbols = 0;
    int dump_rom = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) { fprintf(stderr, "-o requires a filename\n"); exit(1); }
            output_file = argv[i];
        } else if (strcmp(argv[i], "--dump-tokens")  == 0) { dump_tokens  = 1; }
        else if   (strcmp(argv[i], "--dump-ast")     == 0) { dump_ast     = 1; }
        else if   (strcmp(argv[i], "--dump-symbols") == 0) { dump_symbols = 1; }
        else if   (strcmp(argv[i], "--dump-rom")     == 0) { dump_rom     = 1; }
        else if   (strcmp(argv[i], "-h") == 0 ||
                   strcmp(argv[i], "--help") == 0) { usage(argv[0]); }
        else if   (argv[i][0] == '-') {
            fprintf(stderr, "unknown option: %s\n", argv[i]); exit(1);
        } else {
            if (input_file) { fprintf(stderr, "multiple input files not supported\n"); exit(1); }
            input_file = argv[i];
        }
    }

    if (!input_file) { fprintf(stderr, "no input file specified\n"); usage(argv[0]); }

    /*
        Get source file
    */

    char* source = read_file(input_file);

    /*
        Lex
    */
    Lexer lexer;
    lexer_init(&lexer, source);

    if (dump_tokens) {
        printf("=== Token Stream ===\n");
        Lexer tmp = lexer;
        Token t;
        do {
            t = lexer_next(&tmp);
            printf("  line=%-3d  %-14s", t.line, token_type_name(t.type));
            if (t.type == TOKEN_NUMBER)   printf("  %u (0x%X)", t.value.number, t.value.number);
            if (t.type == TOKEN_REGISTER) printf("  v%X", t.value.reg);
            if (t.type == TOKEN_IDENT)    printf("  '%s'", t.value.ident);
            printf("\n");
        } while (t.type != TOKEN_EOF);
        free(source);
        return 0;
    }

    /*
        Parse
    */

    Parser parser;
    parser_init(&parser, &lexer);
    Program prog = parser_parse(&parser);

        if (dump_ast) {
        program_dump(&prog);
        program_free(&prog);
        free(source);
        return 0;
    }

    /*
        Codegen
    */
   SymTable st;
   symtable_init(&st);

   ROM rom;
   codegen_run(&prog, &st, &rom);

    if (dump_symbols) symtable_dump(&st);
    if (dump_rom)     rom_dump(&rom);

    /*
        Write ROM
    */
    if (rom_write(&rom, output_file) != 0) {
        program_free(&prog);
        free(source);
        return 1;
    }

   printf("Compiled %s → %s  (%zu bytes)\n", input_file, output_file, rom.size);

    program_free(&prog);
    free(source);
    return 0;
}