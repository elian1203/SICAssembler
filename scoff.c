#include <string.h>
#include <stdlib.h>
#include "scoff.h"

int getInstructionFormat(char *opcode) {
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

void getInstructionCode(struct SymbolTable *symbolTable, char *code, unsigned long memoryLocation, char *opcode,
                        char *operand, char **modifications, int *numModifications) {
    unsigned long hex = getOpcodeHex(opcode);
    int numBytes = getInstructionFormat(opcode);

    int length = snprintf(code, 1024, "T%06lX%02X", memoryLocation, numBytes);

    if (numBytes == 1) {
        // format 1
        length += snprintf(code + length * sizeof(char), 1024 - length * sizeof(char), "%02lX", hex);
    } else if (numBytes == 2) {
        // format 2
        int r1, r2;
        switch (operand[0]) {
            case 'A':
                r1 = 0;
                break;
            case 'X':
                r1 = 1;
                break;
            case 'L':
                r1 = 2;
        }

        // if there is no second register specified just put 0, otherwise put the appropriate register number
        if (operand[1] != ',') {
            r2 = 0;
        } else {
            switch (operand[2]) {
                case 'B':
                    r2 = 3;
                    break;
                case 'S':
                    r2 = 4;
                    break;
                case 'T':
                    r2 = 5;
                    break;
                case 'F':
                    r2 = 6;
            }
        }
        length += snprintf(code + length * sizeof(char), 1024 - length * sizeof(char), "%02lX%1X%1X", hex, r1, r2);
    } else if (numBytes == 3) {
        // TODO: format 3
        int n = 0;
        int i = 0;
        if(operand[0] == '#'){
            n = 0;
            i = 1;
        } else if(operand[0] == '@'){
            n = 2;
            i = 0;
        }
        hex = hex<<2;
        hex += n + i;
        length += snprintf(code + length * sizeof(char), 1024 - length * sizeof(char), "%02lX", hex);
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
    } else {
        // TODO: format 4
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

void createObjectFile(FILE *inputFile, char *outputFileName, struct SymbolTable *symbolTable) {
    int i;

    int numTextRecords = 0;
    int numModifications = 0;

    char *textRecords[1024];
    char *modifications[1024];

    memset(textRecords, 0, 1024 * sizeof(char *));
    memset(modifications, 0, 1024 * sizeof(char *));

    int lineNumber = 0;
    char line[1024];

    // 0 1 2
    char str1[1024], str2[1024], str3[1024], code[1024];
    int currentString = 0;
    int currentStringIndex = 0;
    int numWords = 0;

    memset(str1, 0, 1024 * sizeof(char));
    memset(str2, 0, 1024 * sizeof(char));
    memset(str3, 0, 1024 * sizeof(char));
    memset(code, 0, 1024 * sizeof(char));

    while (fgets(line, 1024, inputFile) != NULL) {
        lineNumber++;

        if (line[0] == '#') { // if line starts with # it is a comment => ignore
            continue;
        }

        parseLineToStrings(line, lineNumber, str1, str2, str3, &currentString, &currentStringIndex, &numWords);

        if (isSICInstruction(str1)) {
            getInstructionCode(symbolTable, code, symbolTable->memoryLocations[lineNumber - 1], str1, str2,
                               modifications, &numModifications);
        } else if (isSICInstruction(str2)) {
            getInstructionCode(symbolTable, code, symbolTable->memoryLocations[lineNumber - 1], str2, str3,
                               modifications, &numModifications);
        } else if (isSICDirective(str1)) {
            getDirectiveCode(symbolTable, code, symbolTable->memoryLocations[lineNumber - 1], str1, str2);
        } else if (isSICDirective(str2)) {
            getDirectiveCode(symbolTable, code, symbolTable->memoryLocations[lineNumber - 1], str2, str3);
        }

        if (strlen(code)) {
//            printf("%s", code);
            char *temp = strdup(code);
            textRecords[numTextRecords++] = temp;
        }

        // prepare strings for next line
        memset(str1, 0, 1024 * sizeof(char));
        memset(str2, 0, 1024 * sizeof(char));
        memset(str3, 0, 1024 * sizeof(char));
        memset(code, 0, 1024 * sizeof(char));
    }

    // print to file
    FILE *outputFile;
    openFile(&outputFile, outputFileName, "w");

    fprintf(outputFile, "H");
    fprintf(outputFile, "%-7s", symbolTable->programName);
    fprintf(outputFile, "%06lX", symbolTable->startingMemoryLocation);
    fprintf(outputFile, "%06lX\r\n", symbolTable->totalMemoryUsage);
    for (i = 0; i < numTextRecords; i++) {
        fprintf(outputFile, "%s", textRecords[i]);
        free(textRecords[i]);
    }
    for (i = 0; i < numModifications; i++) {
//        printf("%s", modifications[i]);
        fprintf(outputFile, "%s", modifications[i]);
        free(modifications[i]);
    }
    fprintf(outputFile, "E%06lX\r\n", symbolTable->firstInstruction);
}
