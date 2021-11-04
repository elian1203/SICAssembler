#include <string.h>
#include <stdlib.h>
#include "scoff.h"

void getInstructionCode(struct SymbolTable *symbolTable, char *code, unsigned long memoryLocation, char *opcode,
                        char *operand, char **modifications, int *numModifications) {
    unsigned long hex = getOpcodeHex(opcode);
    int numBytes = 3;
    int length = snprintf(code, 1024, "T%06lX%02X%02lX", memoryLocation, numBytes, hex);
    if (strlen(operand) > 0) {
        unsigned long symbolLocation;
        if (stringContainsChar(operand, ',')) {
            int i = 0;
            while (operand[i++] != ',');
            // i - 1 is now the index of ','
            operand[i - 1] = 0;

            // add 8000 since there is a comma
            symbolLocation = getSymbolMemoryLocation(symbolTable, operand) + 8000;
            if (symbolLocation == 7999) {
                printf("ERROR invalid symbol specified (%s)\n", operand);
                exit(1);
            }
        } else {
            symbolLocation = getSymbolMemoryLocation(symbolTable, operand);
            if (symbolLocation == -1) {
                printf("ERROR invalid symbol specified (%s)\n", operand);
                exit(1);
            }
        }

        length += snprintf(code + (length * sizeof(char)), 1024 - (length * sizeof(char)), "%04lX",
                           symbolLocation);

        // add necessary modification record
        char *modification = malloc(19 * sizeof(char));
//        snprintf(modification, 18 * sizeof(char), "M%06lX%02X+%06lX\n", memoryLocation + 1, 4,
//                 symbolTable->startingMemoryLocation);
        snprintf(modification, 19 * sizeof(char), "M%06lX%02X+%-6s\r\n", memoryLocation + 1, 4,
                 symbolTable->symbols[0].name);

        modifications[*numModifications] = modification;
        *numModifications += 1;
    } else {
        length += snprintf(code + (length * sizeof(char)), 1024 - (length * sizeof(char)), "%04X", 0);
    }

    snprintf(code + (length * sizeof(char)), 1024 - (length * sizeof(char)), "\r\n");
}

void getDirectiveCode(struct SymbolTable *symbolTable, char *code, unsigned long memoryLocation, char *directive,
                      char *operand) {
    char append[128];
    memset(append, 0, 128 * sizeof(char));
    int bytesAppended = 0;

    int length = 0;

    if (!strcmp(directive, "BYTE")) {
        if (operand[0] == 'C') {
            int c = 2;
            while (operand[c] != '\'') {
                length += snprintf(append + (length * sizeof(char)), 128 - (length * sizeof(char)), "%02hhX",
                                   operand[c]);
                c++;
            }

            bytesAppended = c - 2;
        } else if (operand[0] == 'X') {
            int c = 2;
            while (operand[c] != '\'') {
                length += snprintf(append + (length * sizeof(char)), 128 - (length * sizeof(char)), "%c", operand[c]);
                c++;
            }

            int hexChars = c - 2;
            bytesAppended = hexChars / 2 + (hexChars % 2);
        }
    } else if (!strcmp(directive, "WORD")) {
        long word = strtol(operand, NULL, 10);
        snprintf(append, 128, "%06lX", word);
        bytesAppended = 3;
    } else if (!strcmp(directive, "END")) {
        if (strlen(operand) > 0 && getSymbolMemoryLocation(symbolTable, operand) == -1) {
            printf("ERROR invalid symbol specified (%s)\n", operand);
            exit(1);
        }
    }

    if (bytesAppended > 0) {
        // a fancy way of detecting overflow on text record length
        length = 0;
        unsigned long currentMemoryLocation = memoryLocation;
        unsigned long remainingBytes = bytesAppended;
        char *ptr = &append[0];
        int lines = 0;

        while (remainingBytes > 0) {
            unsigned long appendBytes = remainingBytes > 30 ? 30 : remainingBytes;
            remainingBytes -= appendBytes;
            length += snprintf(code + (length * sizeof(char)), 1024 - (length * sizeof(char)),
                               "T%06lX%02lX%.60s\r\n", currentMemoryLocation, appendBytes, ptr);

            currentMemoryLocation += appendBytes;
            ptr = &append[++lines * 60];
        }
    }
}

void createObjectFile(FILE *inputFile, FILE *outputFile, struct SymbolTable *symbolTable) {
    int i;
    fprintf(outputFile, "H");
    fprintf(outputFile, "%-7s", symbolTable->programName);
//    printf("%s", symbolTable->programName);
    fprintf(outputFile, "%06lX", symbolTable->startingMemoryLocation);
    fprintf(outputFile, "%06lX\r\n", symbolTable->totalMemoryUsage);

    int numModifications = 0;
    char *modifications[1024];
    memset(modifications, 0, 1024 * sizeof(char *));

    int lineNumber = 0;
    char line[1024];

    // 0 1 2
    char str1[1024], str2[1024], str3[1024], code[1024];
    int currentString = 0;
    int currentStringIndex = 0;
    int numWords = 0;
    int inString = 0;

    memset(str1, 0, 1024 * sizeof(char));
    memset(str2, 0, 1024 * sizeof(char));
    memset(str3, 0, 1024 * sizeof(char));
    memset(code, 0, 1024 * sizeof(char));

    while (fgets(line, 1024, inputFile) != NULL) {
        lineNumber++;

        if (line[0] == '#') { // if line starts with # it is a comment => ignore
            continue;
        }

        parseLineToStrings(line, lineNumber, str1, str2, str3, &currentString, &currentStringIndex, &numWords,
                           &inString);

        if (isSICInstruction(str1)) {
            getInstructionCode(symbolTable, code, symbolTable->memoryLocations[lineNumber - 1], str1, str2,
                               modifications, &numModifications);
//            printf("%s\n", str1);
        } else if (isSICInstruction(str2)) {
//            printf("%s\n", str2);
            getInstructionCode(symbolTable, code, symbolTable->memoryLocations[lineNumber - 1], str2, str3,
                               modifications, &numModifications);
        } else if (isSICDirective(str1)) {
//            printf("%s\n", str1);
            getDirectiveCode(symbolTable, code, symbolTable->memoryLocations[lineNumber - 1], str1, str2);
        } else if (isSICDirective(str2)) {
//            printf("%s\n", str2);
            getDirectiveCode(symbolTable, code, symbolTable->memoryLocations[lineNumber - 1], str2, str3);
        }

        if (strlen(code)) {
//            printf("%s", code);
            fprintf(outputFile, "%s", code);
        }

        // prepare strings for next line
        memset(str1, 0, 1024 * sizeof(char));
        memset(str2, 0, 1024 * sizeof(char));
        memset(str3, 0, 1024 * sizeof(char));
        memset(code, 0, 1024 * sizeof(char));
    }
    for (i = 0; i < numModifications; i++) {
//        printf("%s", modifications[i]);
        fprintf(outputFile, "%s", modifications[i]);
        free(modifications[i]);
    }
    fprintf(outputFile, "E%06lX\r\n", symbolTable->firstInstruction);
}
