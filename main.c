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

    FILE *inputFile = NULL, *outputFile = NULL;

    // initialize table
    struct SymbolTable *symbolTable = NULL;
    symbolTable = malloc(sizeof(struct SymbolTable));

    openFile(&inputFile, argv[1], "r");
    parseSymbolTable(inputFile, symbolTable);
    printSymbolTable(symbolTable);
    fclose(inputFile);

    openFile(&inputFile, argv[1], "r");
    openFile(&outputFile, strcat(argv[1], ".obj"), "w");
    createObjectFile(inputFile, outputFile, symbolTable);
    fclose(inputFile);
    fclose(outputFile);

    freeSymbolTable(symbolTable);
    return 0;
}

void openFile(FILE **file, char *fileName, char *mode) {
    *file = fopen(fileName, mode);

    if (*file == NULL) {
        if (!strcmp(mode, "r"))
            printf("File not found!\r\n");
        else
            printf("Could not open file for writing!\r\n");
        exit(1);
    }
}


