#ifndef ALU_H
#define ALU_H

#include <stdint.h>

// Arithmeticv operations 

void ADD_A(uint8_t val);
void ADC_A(uint8_t val);
void SUB_A(uint8_t val);
void SBC_A(uint8_t val);
void CP_A(uint8_t val);

// Logical Operations

void AND_A(uint8_t val);
void OR_A(uint8_t val);
void XOR_A(uint8_t val);

// misc ALU ops

uint8_t INC(uint8_t val);
uint8_t DEC(uint8_t val);
void DAA();
void CPL();

#endif