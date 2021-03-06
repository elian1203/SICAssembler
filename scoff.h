#ifndef COP3404P2_SCOFF_H
#define COP3404P2_SCOFF_H

#include "symbols.h"

struct opCode {
    char opcode[8];
    unsigned long hex;
};

unsigned long getOpcodeHex(char *opcode);

void createObjectFile(FILE *inputFile, char *outputFileName, struct SymbolTable *symbolTable);

void openFile(FILE **file, char *fileName, char *mode);

#endif //COP3404P2_SCOFF_H
