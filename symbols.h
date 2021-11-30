#ifndef COP3404P1_SYMBOLS_H
#define COP3404P1_SYMBOLS_H

#include <stdio.h>

struct Symbol {
    char *name;
    unsigned long location;
    struct Symbol *address;
};

struct SymbolTable {
    int symbolsInTable;
    int tableSize;
    unsigned long startingMemoryLocation;
    unsigned long firstInstruction;
    unsigned long totalMemoryUsage;
    char *programName;
    struct Symbol *symbols;
    unsigned long memoryLocations[2048];
    unsigned long baseLocation;
};

void parseLineToStrings(char *line, int lineNumber, char *str1, char *str2, char *str3, int *currentString,
                        int *currentStringIndex, int *numWords);

void parseSymbolTable(FILE *file, struct SymbolTable *symbolTable);

void addSymbolToTable(struct Symbol *symbol, struct SymbolTable *symbolTable);

void printSymbolTable(struct SymbolTable *symbolTable);

void freeSymbolTable(struct SymbolTable *symbolTable);

int isSICInstruction(char *input);

int isSICDirective(char *input);

unsigned long getSymbolMemoryLocation(struct SymbolTable *symbolTable, char *name);

int stringContainsChar(char *string, char test);

#endif //COP3404P1_SYMBOLS_H
