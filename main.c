#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbols.h"
#include "scoff.h"

void openFile(FILE **file, char *fileName, char *mode);

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Specify SIC file to parse!: ");
        printf("%s [filename]\r\n", argv[0]);
        return 1;
    }

    FILE *inputFile = NULL;

    // initialize table
    struct SymbolTable *symbolTable = NULL;
    symbolTable = malloc(sizeof(struct SymbolTable));

    openFile(&inputFile, argv[1], "r");
    parseSymbolTable(inputFile, symbolTable);
    fclose(inputFile);

    openFile(&inputFile, argv[1], "r");
    createObjectFile(inputFile, strcat(argv[1], ".obj"), symbolTable);
    fclose(inputFile);

    freeSymbolTable(symbolTable);
    return 0;
}



