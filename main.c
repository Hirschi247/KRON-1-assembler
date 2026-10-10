#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "main.h"
#include "encode/encode.h"
#include "scanner/scanner.h"
#include "symtab/symtab.h"

static char *input_filename;
static char *output_filename;

static char *read_file(const char *filename)
{
    FILE *source = fopen(filename, "r");
    if (source == NULL)
    {
        perror("Error while opening input file");
        exit(-1);
    }

    // Größe der File ermitteln:
    fseek(source, 0, SEEK_END);
    long size = ftell(source);
    fseek(source, 0, SEEK_SET);

    // Speicher für den Dateiinhalt (Source) allokieren:
    char *buffer = malloc(size + 1);
    if (buffer == NULL)
    {
        perror("Error while loading file content in buffer");
        exit(-1);
    }

    size_t read = fread(buffer, 1, size, source);
    buffer[read] = '\0';

    fclose(source);
    return buffer;
}

int main(const int argc, char * argv[]) {
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <input-file> <output-file>\n", argv[0]);
        return 1;
    }
    char *bla = argv[0];
    input_filename = argv[1];
    output_filename = argv[2];

    Dict *symtab = symtab_init();
    Scanner scanner = {0};
    Token token = {0};

    // Pass 1:
    scanner_init(&scanner, read_file(input_filename));

    int instruction = 0;
    int line_has_instruction = 0; // Leerzeilen, reine Kommentar- und Label-Zeilen zählen nicht

    while ((token = scanner_next(&scanner)).type != TOK_EOF)
    {
        if (token.type == TOK_LABEL)
        {
            Label *label = malloc(sizeof(Label));
            label->name = strdup(token.text); // token.text wird beim nächsten scanner_next überschrieben
            label->address = instruction;
            symtab_insert(symtab, label);
        }
        else if (token.type == TOK_EOL)
        {
            if (line_has_instruction)
                instruction++;
            line_has_instruction = 0;
        }
        else if (token.type == TOK_ERROR)
        {
            fprintf(stderr, "Zeile %d: %s\n", token.line, token.text);
            exit(-1);
        }
        else
            line_has_instruction = 1;
    }

    // Pass 2:
    scanner_init(&scanner, read_file(input_filename));

    Token *token_line = malloc(sizeof(*token_line) * MAX_TOKENS_PER_LINE);

    FILE *output_file = fopen(output_filename, "w");
    if (output_file == NULL)
    {
        perror("Error while creating output file");
        exit(-1);
    }

    while ((token = scanner_next(&scanner)).type != TOK_EOF) {
        int counter = 0;
        memset(token_line, 0, sizeof(*token_line) * MAX_TOKENS_PER_LINE); // keine Operanden aus der Vorzeile übernehmen

        if (token.type == TOK_LABEL)
            token = scanner_next(&scanner);

        while (token.type != TOK_EOL && token.type != TOK_EOF)
        {
            int is_operand = token.type == TOK_IDENT || token.type == TOK_REGISTER
                          || token.type == TOK_NUMBER || token.type == TOK_LABEL_IDENT;

            if (is_operand && counter == MAX_TOKENS_PER_LINE)
            {
                fprintf(stderr, "Zeile %d: Zu viele Operanden (maximal %d Tokens pro Zeile)\n", token.line, MAX_TOKENS_PER_LINE);
                exit(-1);
            }

            if (token.type == TOK_IDENT || token.type == TOK_REGISTER || token.type == TOK_NUMBER) {
                token_line[counter] = token;
                counter++;
            }
            else if (token.type == TOK_LABEL_IDENT)
            {
                Label *label = symtab_lookup(symtab, token.text);
                if (label == NULL)
                {
                    fprintf(stderr, "Zeile %d: Label %s not found\n", token.line, token.text);
                    exit(-1);
                }
                token_line[counter].type = TOK_NUMBER;
                token_line[counter].value = label->address;
                token_line[counter].line = token.line;
                counter++;
            }

            token = scanner_next(&scanner);
        }

        if (counter == 0) // Leerzeile, reine Kommentar- oder Label-Zeile
            continue;

        uint64_t *instruction_binary = encode(token_line);
        fprintf(output_file, "%09" PRIX64 "\n", *instruction_binary);
    }
    fclose(output_file);
}