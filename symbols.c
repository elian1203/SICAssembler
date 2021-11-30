#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbols.h"

int symbolContainedInTable(struct SymbolTable *symbolTable, char *name) {
    if (symbolTable->symbolsInTable == 0)
        return 0;

    int i;
    for (i = 0; i < symbolTable->symbolsInTable; i++) {
        if (!strcmp(name, (&symbolTable->symbols[i])->name)) {
            return 1;
        }
    }

    return 0;
}

unsigned long getSymbolMemoryLocation(struct SymbolTable *symbolTable, char *name) {
    int i;
    for (i = 0; i < symbolTable->symbolsInTable; i++) {
        if (!strcmp(name, (&symbolTable->symbols[i])->name)) {
            return symbolTable->symbols[i].location;
        }
    }

    return -1;
}

struct Symbol *createSymbol(char *name, unsigned long location) {
    struct Symbol *symbol = malloc(sizeof(struct Symbol));

    symbol->name = strdup(name);
    symbol->location = location;
    symbol->address = symbol;
    return symbol;
}

void expandSymbolTable(struct SymbolTable *symbolTable) {
    int tableSize = symbolTable->tableSize;

    if (tableSize == 0) {
        tableSize = 5;
    } else {
        tableSize *= 2;
    }

    symbolTable->tableSize = tableSize;

    struct Symbol *temp = malloc(tableSize * sizeof *temp);

    int i;
    for (i = 0; i < symbolTable->symbolsInTable; i++) {
        temp[i] = symbolTable->symbols[i];
    }

    // if greater than 5 it is not original allocation
    if (tableSize > 5) {
        free(symbolTable->symbols);
    }

    symbolTable->symbols = temp;
}

void addSymbolToTable(struct Symbol *symbol, struct SymbolTable *symbolTable) {
    int symbolsInTable = symbolTable->symbolsInTable;

    if (symbolsInTable == symbolTable->tableSize) {
        expandSymbolTable(symbolTable);
    }

    symbolTable->symbols[symbolsInTable] = *symbol;
    symbolTable->symbolsInTable += 1;
}

int isStringUppercase(char *string) {
    if (string == NULL)
        return 1;

    int i = 0;
    char c;
    while ((c = string[i++]) != '\0') {
        if (c >= 97 && c <= 122) {
            return 0;
        }
    }

    return 1;
}

void uppercaseString(char *string) {
    if (string == NULL)
        return;

    int i = 0;
    char c;
    while ((c = string[i]) != '\0') {
        if (c >= 97 && c <= 122) {
            string[i] = (int) c - 32;
        }
        i++;
    }
}

int stringContainsChar(char *string, char test) {
    if (string == NULL)
        return 0;

    int i = 0;
    char c;
    while ((c = string[i++]) != '\0') {
        if (c == test) {
            return 1;
        }
    }

    return 0;
}

int validHex(char *string) {
    if (string == NULL)
        return 0;

    char *copy;
    copy = strdup(string);
    uppercaseString(copy);


    int i = 0;
    char c;
    while ((c = copy[i++]) != 0) {
        if (!((c >= 48 && c <= 57) || c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F')) {
            free(copy);
            return 0;
        }
    }

    free(copy);

    return 1;
}

void validateSymbol(int lineNumber, char *symbol, struct SymbolTable *symbolTable) {
    // symbol max length is 6 chars
    unsigned long length = strlen(symbol);
    if (length < 1 || length > 6) {
        printf("Line %d ERROR: Invalid symbol name! Symbol length invalid (max 6 characters)!\r\n", lineNumber);
        exit(1);
    }

    // symbol already in table
    if (symbolContainedInTable(symbolTable, symbol)) {
        // symbol already exists
        printf("Line %d ERROR: Symbol already defined!\r\n", lineNumber);
        exit(1);
    }

    // symbol must start with alpha char
    if (symbol[0] < 65 || symbol[0] > 90) {
        printf("Line %d ERROR: Invalid symbol name! Symbol must start with alpha character (A-Z)!\r\n", lineNumber);
        exit(1);
    }

    // symbol cannot contain some chars "Symbols cannot contain spaces, $, !, =, +, - , (,  ), or@"
    if (stringContainsChar(symbol, ' ')
        || stringContainsChar(symbol, '$')
        || stringContainsChar(symbol, '!')
        || stringContainsChar(symbol, '=')
        || stringContainsChar(symbol, '+')
        || stringContainsChar(symbol, '-')
        || stringContainsChar(symbol, '(')
        || stringContainsChar(symbol, ')')
        || stringContainsChar(symbol, '@')) {
        printf("Line %d ERROR: Invalid symbol name! Symbols cannot contain spaces, $, !, =, +, - , (,  ), or @!\r\n",
               lineNumber);
        exit(1);
    }

    // check if symbol is directive
    if (isSICDirective(symbol)) {
        printf("Line %d ERROR: Invalid symbol name! Symbol name is a directive!\r\n", lineNumber);
        exit(1);
    }

    if (!isStringUppercase(symbol)) {
        printf("Line %d ERROR: Invalid instruction or directive! (%s) must be uppcase!\r\n", lineNumber, symbol);
        exit(1);
    }
}

void handleDirective(int lineNumber, struct SymbolTable *symbolTable, unsigned long *currentMemoryLocation,
                     char *directive, char *operand) {
    if (!strcmp(directive, "START")) {
        // start directive -> set starting memory location
        symbolTable->startingMemoryLocation = strtol(operand, NULL, 16);
    } else if (!strcmp(directive, "RESB")) {
        long bytes = strtol(operand, NULL, 10);
        *currentMemoryLocation += bytes;
    } else if (!strcmp(directive, "BYTE")) {
        if (operand[0] == 'C') {
            int c = 2;
            while (operand[c++] != '\'');

            *currentMemoryLocation += c - 3;
        } else if (operand[0] == 'X') {
            char hex[9];
            memset(hex, 0, 8 * sizeof(char));

            int c = 2;
            while (operand[c] != '\'') {
                if (c > 10) {
                    printf("Line %d ERROR: Invalid hex provided!\r\n", lineNumber);
                    exit(1);
                } else {
                    hex[c - 2] = operand[c];
                }

                c++;
            }

            // hex bytes is the ceil of the hex length / 2 => AE: 1 BDCE: 2 BDC: 2
            int length = c - 2;
            int hexBytes = length / 2 + (length % 2);

            if (!validHex(hex)) {
                printf("Line %d ERROR: Invalid hex provided!\r\n", lineNumber);
                exit(1);
            }

            *currentMemoryLocation += hexBytes;
        } else {
            *currentMemoryLocation += 1;
        }
    } else if (!strcmp(directive, "WORD")) {
        long word = strtol(operand, NULL, 10);
        if (word > 8388607 || word < -8388608) {
            printf("Line %d ERROR: Invalid word! Word is greater than 24 bit requirement!\r\n", lineNumber);
            exit(1);
        }
        *currentMemoryLocation += 3;
    } else if (!strcmp(directive, "RESW")) {
        long words = strtol(operand, NULL, 10);
        *currentMemoryLocation += words * 3;
    } else if (!strcmp(directive, "RESR") || !strcmp(directive, "EXPORTS")) {
        *currentMemoryLocation += 3;
    } else if (!strcmp(directive, "BASE")) {
        symbolTable->baseLocation = getSymbolMemoryLocation(operand);
    }
}


void parseLineToStrings(char *line, int lineNumber, char *str1, char *str2, char *str3, int *currentString,
                        int *currentStringIndex, int *numWords) {
    int charIndex = 0;
    int inString = 0;
    char c;
    while (1) {
        c = line[charIndex];
        if (c == '\n') {
            if (currentString == 0 && currentStringIndex == 0) {
                printf("Line %d ERROR: Empty lines are not valid! Use comment instead!\r\n", lineNumber);
                exit(1);
            }

            *numWords += 1;
            *currentString = 0;
            *currentStringIndex = 0;

            break;
        } else if (c == '\0') {
            if (*currentStringIndex > 0) {
                *currentString += 1;
                *numWords += 1;
                *currentStringIndex = 0;
            }
            break;
        } else if ((c == ' ' && inString == 0) || c == '\t' || c == '\r') {
            if (*currentStringIndex > 0) {
                *currentString += 1;
                *numWords += 1;
                *currentStringIndex = 0;
            }
        } else {
            if (c == '\'') {
                if (inString) inString = 0;
                else inString = 1;
            }

            if (*currentString == 0) {
                str1[*currentStringIndex] = c;
                *currentStringIndex += 1;
            } else if (*currentString == 1) {
                str2[*currentStringIndex] = c;
                *currentStringIndex += 1;
            } else if (*currentString == 2) {
                str3[*currentStringIndex] = c;
                *currentStringIndex += 1;
            }
        }

        charIndex++;
    }
}

int getInstructionBytes(char *opcode) {
    if (opcode[0] == '+') {
        return 4;
    } else {
        if (!strcmp(opcode, "FIX")
            || !strcmp(opcode, "FLOAT")
            || !strcmp(opcode, "HIO")
            || !strcmp(opcode, "NORM")
            || !strcmp(opcode, "SIO")
            || !strcmp(opcode, "TIO")) {
            return 1;
        } else if (!strcmp(opcode, "ADDR")
                   || !strcmp(opcode, "CLEAR")
                   || !strcmp(opcode, "COMPR")
                   || !strcmp(opcode, "DIVR")
                   || !strcmp(opcode, "MULR")
                   || !strcmp(opcode, "RMO")
                   || !strcmp(opcode, "SHIFTL")
                   || !strcmp(opcode, "SHIFTR")
                   || !strcmp(opcode, "SUBR")
                   || !strcmp(opcode, "SVC")
                   || !strcmp(opcode, "TIXR")) {
            return 2;
        } else {
            return 3;
        }
    }
}

void parseSymbolTable(FILE *file, struct SymbolTable *symbolTable) {
    symbolTable->symbolsInTable = 0;
    symbolTable->tableSize = 0;
    symbolTable->startingMemoryLocation = -1;
    symbolTable->firstInstruction = -1;

    int lineNumber = 0;
    char line[1024];

    unsigned long currentMemoryLocation;
    currentMemoryLocation = 0;

    // 0 1 2
    char str1[1024], str2[1024], str3[1024];
    int currentString = 0;
    int currentStringIndex = 0;
    int numWords = 0;

    memset(str1, 0, 1024 * sizeof(char));
    memset(str2, 0, 1024 * sizeof(char));
    memset(str3, 0, 1024 * sizeof(char));

    while (fgets(line, 1024, file) != NULL) {
        // set memory location for this line
        symbolTable->memoryLocations[lineNumber] = currentMemoryLocation;

        lineNumber++;
        if (line[0] == '#') { // if line starts with # it is a comment => ignore
            continue;
        }

        parseLineToStrings(line, lineNumber, str1, str2, str3, &currentString, &currentStringIndex, &numWords);

//        printf("1|%s| 2|%s| 3|%s| %d\n", str1, str2, str3, numWords);

        // empty line -> ignore
        if (numWords == 0) {
            continue;
        } else if (numWords == 1) {
            if (!isSICInstruction(str1)) {
                printf("Line %d ERROR: Not a SIC instruction!\r\n", lineNumber);
                exit(1);
            }
            if (symbolTable->firstInstruction == -1)
                symbolTable->firstInstruction = currentMemoryLocation;
            currentMemoryLocation += getInstructionBytes(str1);
        } else if (numWords == 2) {
            if (isSICInstruction(str1)) {
                if (symbolTable->firstInstruction == -1)
                    symbolTable->firstInstruction = currentMemoryLocation;
                currentMemoryLocation += getInstructionBytes(str1);
            } else if (isSICDirective(str1)) {
                handleDirective(lineNumber, symbolTable, &currentMemoryLocation, str1, str2);
            } else if (isSICInstruction(str2)) {
                if (symbolTable->firstInstruction == -1)
                    symbolTable->firstInstruction = currentMemoryLocation;
                // new symbol
                validateSymbol(lineNumber, str1, symbolTable);
                struct Symbol *symbol = createSymbol(str1, currentMemoryLocation);
                addSymbolToTable(symbol, symbolTable);

                currentMemoryLocation += getInstructionBytes(str2);
            } else {
                printf("Line %d ERROR: Invalid line!\r\n", lineNumber);
                exit(1);
            }
        } else {
            if (isSICDirective(str2)) {
                validateSymbol(lineNumber, str1, symbolTable);
                struct Symbol *symbol = createSymbol(str1, currentMemoryLocation);
                addSymbolToTable(symbol, symbolTable);

                handleDirective(lineNumber, symbolTable, &currentMemoryLocation, str2, str3);
                if (!strcmp(str2, "START")) {
                    symbolTable->programName = strdup(str1);
                }
            } else if (isSICInstruction(str2)) {
                if (symbolTable->firstInstruction == -1)
                    symbolTable->firstInstruction = currentMemoryLocation;
                validateSymbol(lineNumber, str1, symbolTable);
                struct Symbol *symbol = createSymbol(str1, currentMemoryLocation);
                addSymbolToTable(symbol, symbolTable);

                currentMemoryLocation += getInstructionBytes(str2);
            } else if (isSICInstruction(str1)) {
                if (symbolTable->firstInstruction == -1)
                    symbolTable->firstInstruction = currentMemoryLocation;
                // no symbol
                currentMemoryLocation += getInstructionBytes(str1);
            } else if (isSICDirective(str1)) {
                // no symbol
                handleDirective(lineNumber, symbolTable, &currentMemoryLocation, str1, str2);
            } else {
                printf("Line %d ERROR: Invalid line!\r\n", lineNumber);
                exit(1);
            }
        }

        if (currentMemoryLocation > 32768) {
            printf("Line %d ERROR: SIC memory exceeded!\r\n", lineNumber);
            exit(1);
        }

        // prepare for next line
        memset(str1, 0, 1024 * sizeof(char));
        memset(str2, 0, 1024 * sizeof(char));
        memset(str3, 0, 1024 * sizeof(char));
        numWords = 0;
    }

    // one final check for memory exceed
    if (currentMemoryLocation + symbolTable->startingMemoryLocation > 32768) {
        printf("Line %d ERROR: SIC memory exceeded!\r\n", lineNumber);
        exit(1);
    }

    if (symbolTable->startingMemoryLocation == -1) {
        printf("Line %d ERROR: No START directive provided!\r\n", lineNumber);
        exit(1);
    }

    int i;
    // update locations to account for starting location
    if (symbolTable->startingMemoryLocation != 0) {
        for (i = 0; i < symbolTable->symbolsInTable; i++) {
            symbolTable->symbols[i].location += symbolTable->startingMemoryLocation;
        }
    }
    symbolTable->firstInstruction += symbolTable->startingMemoryLocation;
    for (i = 0; i < 2048; i++)
        symbolTable->memoryLocations[i] += symbolTable->startingMemoryLocation;

    // set total memory used
    symbolTable->totalMemoryUsage = currentMemoryLocation;
}

void printSymbolTable(struct SymbolTable *symbolTable) {
    int i;
    for (i = 0; i < symbolTable->symbolsInTable; i++) {
        printf("%s\t%lX\r\n", symbolTable->symbols[i].name, symbolTable->symbols[i].location);
    }
}

void freeSymbolTable(struct SymbolTable *symbolTable) {
    int i;
    for (i = 0; i < symbolTable->symbolsInTable; i++) {
        free(symbolTable->symbols[i].name);
        free(symbolTable->symbols[i].address);
    }
    free(symbolTable->programName);
    free(symbolTable->symbols);
    free(symbolTable);
}
