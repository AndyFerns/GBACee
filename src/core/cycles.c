#include "cycles.h"

// ---------------------------------------------------------------------------
// Base opcode cycle counts (clock cycles, 1 machine cycle = 4 clock cycles)
//
// For conditional instructions (JP cc, JR cc, CALL cc, RET cc), two values
// exist: taken vs not-taken. This table stores the TAKEN (longer) value.
// Use a separate branch_cycles[] table (below) for the not-taken value.
//
// Source: Game Boy CPU Manual v1.01 (DP & Others, 1999)
//         SBC A,# (0xDE) cycle count sourced from Pan Docs / verified hardware
// ---------------------------------------------------------------------------
const uint8_t opcode_cycles[256] = {
    [0x00] = 4,   // NOP
    [0x01] = 12,  // LD BC,nn
    [0x02] = 8,   // LD (BC),A
    [0x03] = 8,   // INC BC
    [0x04] = 4,   // INC B
    [0x05] = 4,   // DEC B
    [0x06] = 8,   // LD B,n
    [0x07] = 4,   // RLCA
    [0x08] = 20,  // LD (nn),SP
    [0x09] = 8,   // ADD HL,BC
    [0x0A] = 8,   // LD A,(BC)
    [0x0B] = 8,   // DEC BC
    [0x0C] = 4,   // INC C
    [0x0D] = 4,   // DEC C
    [0x0E] = 8,   // LD C,n
    [0x0F] = 4,   // RRCA

    [0x10] = 4,   // STOP
    [0x11] = 12,  // LD DE,nn
    [0x12] = 8,   // LD (DE),A
    [0x13] = 8,   // INC DE
    [0x14] = 4,   // INC D
    [0x15] = 4,   // DEC D
    [0x16] = 8,   // LD D,n
    [0x17] = 4,   // RLA
    [0x18] = 12,  // JR n  (always taken, +4 over not-taken of 8)
    [0x19] = 8,   // ADD HL,DE
    [0x1A] = 8,   // LD A,(DE)
    [0x1B] = 8,   // DEC DE
    [0x1C] = 4,   // INC E
    [0x1D] = 4,   // DEC E
    [0x1E] = 8,   // LD E,n
    [0x1F] = 4,   // RRA

    [0x20] = 12,  // JR NZ,* (taken=12, not-taken=8)
    [0x21] = 12,  // LD HL,nn
    [0x22] = 8,   // LDI (HL),A  [LD (HL+),A]
    [0x23] = 8,   // INC HL
    [0x24] = 4,   // INC H
    [0x25] = 4,   // DEC H
    [0x26] = 8,   // LD H,n
    [0x27] = 4,   // DAA
    [0x28] = 12,  // JR Z,*  (taken=12, not-taken=8)
    [0x29] = 8,   // ADD HL,HL
    [0x2A] = 8,   // LDI A,(HL)  [LD A,(HL+)]
    [0x2B] = 8,   // DEC HL
    [0x2C] = 4,   // INC L
    [0x2D] = 4,   // DEC L
    [0x2E] = 8,   // LD L,n
    [0x2F] = 4,   // CPL

    [0x30] = 12,  // JR NC,*  (taken=12, not-taken=8)
    [0x31] = 12,  // LD SP,nn
    [0x32] = 8,   // LDD (HL),A  [LD (HL-),A]
    [0x33] = 8,   // INC SP
    [0x34] = 12,  // INC (HL)
    [0x35] = 12,  // DEC (HL)
    [0x36] = 12,  // LD (HL),n
    [0x37] = 4,   // SCF
    [0x38] = 12,  // JR C,*   (taken=12, not-taken=8)
    [0x39] = 8,   // ADD HL,SP
    [0x3A] = 8,   // LDD A,(HL)  [LD A,(HL-)]
    [0x3B] = 8,   // DEC SP
    [0x3C] = 4,   // INC A
    [0x3D] = 4,   // DEC A
    [0x3E] = 8,   // LD A,#
    [0x3F] = 4,   // CCF

    [0x40] = 4,   // LD B,B
    [0x41] = 4,   // LD B,C
    [0x42] = 4,   // LD B,D
    [0x43] = 4,   // LD B,E
    [0x44] = 4,   // LD B,H
    [0x45] = 4,   // LD B,L
    [0x46] = 8,   // LD B,(HL)
    [0x47] = 4,   // LD B,A
    [0x48] = 4,   // LD C,B
    [0x49] = 4,   // LD C,C
    [0x4A] = 4,   // LD C,D
    [0x4B] = 4,   // LD C,E
    [0x4C] = 4,   // LD C,H
    [0x4D] = 4,   // LD C,L
    [0x4E] = 8,   // LD C,(HL)
    [0x4F] = 4,   // LD C,A

    [0x50] = 4,   // LD D,B
    [0x51] = 4,   // LD D,C
    [0x52] = 4,   // LD D,D
    [0x53] = 4,   // LD D,E
    [0x54] = 4,   // LD D,H
    [0x55] = 4,   // LD D,L
    [0x56] = 8,   // LD D,(HL)
    [0x57] = 4,   // LD D,A
    [0x58] = 4,   // LD E,B
    [0x59] = 4,   // LD E,C
    [0x5A] = 4,   // LD E,D
    [0x5B] = 4,   // LD E,E
    [0x5C] = 4,   // LD E,H
    [0x5D] = 4,   // LD E,L
    [0x5E] = 8,   // LD E,(HL)
    [0x5F] = 4,   // LD E,A

    [0x60] = 4,   // LD H,B
    [0x61] = 4,   // LD H,C
    [0x62] = 4,   // LD H,D
    [0x63] = 4,   // LD H,E
    [0x64] = 4,   // LD H,H
    [0x65] = 4,   // LD H,L
    [0x66] = 8,   // LD H,(HL)
    [0x67] = 4,   // LD H,A
    [0x68] = 4,   // LD L,B
    [0x69] = 4,   // LD L,C
    [0x6A] = 4,   // LD L,D
    [0x6B] = 4,   // LD L,E
    [0x6C] = 4,   // LD L,H
    [0x6D] = 4,   // LD L,L
    [0x6E] = 8,   // LD L,(HL)
    [0x6F] = 4,   // LD L,A

    [0x70] = 8,   // LD (HL),B
    [0x71] = 8,   // LD (HL),C
    [0x72] = 8,   // LD (HL),D
    [0x73] = 8,   // LD (HL),E
    [0x74] = 8,   // LD (HL),H
    [0x75] = 8,   // LD (HL),L
    [0x76] = 4,   // HALT
    [0x77] = 8,   // LD (HL),A
    [0x78] = 4,   // LD A,B
    [0x79] = 4,   // LD A,C
    [0x7A] = 4,   // LD A,D
    [0x7B] = 4,   // LD A,E
    [0x7C] = 4,   // LD A,H
    [0x7D] = 4,   // LD A,L
    [0x7E] = 8,   // LD A,(HL)
    [0x7F] = 4,   // LD A,A

    [0x80] = 4,   // ADD A,B
    [0x81] = 4,   // ADD A,C
    [0x82] = 4,   // ADD A,D
    [0x83] = 4,   // ADD A,E
    [0x84] = 4,   // ADD A,H
    [0x85] = 4,   // ADD A,L
    [0x86] = 8,   // ADD A,(HL)
    [0x87] = 4,   // ADD A,A
    [0x88] = 4,   // ADC A,B
    [0x89] = 4,   // ADC A,C
    [0x8A] = 4,   // ADC A,D
    [0x8B] = 4,   // ADC A,E
    [0x8C] = 4,   // ADC A,H
    [0x8D] = 4,   // ADC A,L
    [0x8E] = 8,   // ADC A,(HL)
    [0x8F] = 4,   // ADC A,A

    [0x90] = 4,   // SUB B
    [0x91] = 4,   // SUB C
    [0x92] = 4,   // SUB D
    [0x93] = 4,   // SUB E
    [0x94] = 4,   // SUB H
    [0x95] = 4,   // SUB L
    [0x96] = 8,   // SUB (HL)
    [0x97] = 4,   // SUB A
    [0x98] = 4,   // SBC A,B
    [0x99] = 4,   // SBC A,C
    [0x9A] = 4,   // SBC A,D
    [0x9B] = 4,   // SBC A,E
    [0x9C] = 4,   // SBC A,H
    [0x9D] = 4,   // SBC A,L
    [0x9E] = 8,   // SBC A,(HL)
    [0x9F] = 4,   // SBC A,A

    [0xA0] = 4,   // AND B
    [0xA1] = 4,   // AND C
    [0xA2] = 4,   // AND D
    [0xA3] = 4,   // AND E
    [0xA4] = 4,   // AND H
    [0xA5] = 4,   // AND L
    [0xA6] = 8,   // AND (HL)
    [0xA7] = 4,   // AND A
    [0xA8] = 4,   // XOR B
    [0xA9] = 4,   // XOR C
    [0xAA] = 4,   // XOR D
    [0xAB] = 4,   // XOR E
    [0xAC] = 4,   // XOR H
    [0xAD] = 4,   // XOR L
    [0xAE] = 8,   // XOR (HL)
    [0xAF] = 4,   // XOR A

    [0xB0] = 4,   // OR B
    [0xB1] = 4,   // OR C
    [0xB2] = 4,   // OR D
    [0xB3] = 4,   // OR E
    [0xB4] = 4,   // OR H
    [0xB5] = 4,   // OR L
    [0xB6] = 8,   // OR (HL)
    [0xB7] = 4,   // OR A
    [0xB8] = 4,   // CP B
    [0xB9] = 4,   // CP C
    [0xBA] = 4,   // CP D
    [0xBB] = 4,   // CP E
    [0xBC] = 4,   // CP H
    [0xBD] = 4,   // CP L
    [0xBE] = 8,   // CP (HL)
    [0xBF] = 4,   // CP A

    [0xC0] = 20,  // RET NZ  (taken=20, not-taken=8)
    [0xC1] = 12,  // POP BC
    [0xC2] = 16,  // JP NZ,nn  (taken=16, not-taken=12)
    [0xC3] = 16,  // JP nn
    [0xC4] = 24,  // CALL NZ,nn  (taken=24, not-taken=12)
    [0xC5] = 16,  // PUSH BC
    [0xC6] = 8,   // ADD A,#
    [0xC7] = 32,  // RST 00H
    [0xC8] = 20,  // RET Z   (taken=20, not-taken=8)
    [0xC9] = 16,  // RET
    [0xCA] = 16,  // JP Z,nn   (taken=16, not-taken=12)
    [0xCB] = 4,   // CB prefix (cycle cost accounted in cb_opcode_cycles)
    [0xCC] = 24,  // CALL Z,nn   (taken=24, not-taken=12)
    [0xCD] = 24,  // CALL nn
    [0xCE] = 8,   // ADC A,#
    [0xCF] = 32,  // RST 08H

    [0xD0] = 20,  // RET NC  (taken=20, not-taken=8)
    [0xD1] = 12,  // POP DE
    [0xD2] = 16,  // JP NC,nn  (taken=16, not-taken=12)
    [0xD3] = 0,   // UNUSED
    [0xD4] = 24,  // CALL NC,nn  (taken=24, not-taken=12)
    [0xD5] = 16,  // PUSH DE
    [0xD6] = 8,   // SUB #
    [0xD7] = 32,  // RST 10H
    [0xD8] = 20,  // RET C   (taken=20, not-taken=8)
    [0xD9] = 16,  // RETI
    [0xDA] = 16,  // JP C,nn   (taken=16, not-taken=12)
    [0xDB] = 0,   // UNUSED
    [0xDC] = 24,  // CALL C,nn   (taken=24, not-taken=12)
    [0xDD] = 0,   // UNUSED
    [0xDE] = 8,   // SBC A,#  (not in manual; 8 cycles per Pan Docs / verified hardware)
    [0xDF] = 32,  // RST 18H

    [0xE0] = 12,  // LDH (n),A  [LD ($FF00+n),A]
    [0xE1] = 12,  // POP HL
    [0xE2] = 8,   // LD (C),A  [LD ($FF00+C),A]
    [0xE3] = 0,   // UNUSED
    [0xE4] = 0,   // UNUSED
    [0xE5] = 16,  // PUSH HL
    [0xE6] = 8,   // AND #
    [0xE7] = 32,  // RST 20H
    [0xE8] = 16,  // ADD SP,#
    [0xE9] = 4,   // JP (HL)
    [0xEA] = 16,  // LD (nn),A
    [0xEB] = 0,   // UNUSED
    [0xEC] = 0,   // UNUSED
    [0xED] = 0,   // UNUSED
    [0xEE] = 8,   // XOR #
    [0xEF] = 32,  // RST 28H

    [0xF0] = 12,  // LDH A,(n)  [LD A,($FF00+n)]
    [0xF1] = 12,  // POP AF
    [0xF2] = 8,   // LD A,(C)  [LD A,($FF00+C)]
    [0xF3] = 4,   // DI
    [0xF4] = 0,   // UNUSED
    [0xF5] = 16,  // PUSH AF
    [0xF6] = 8,   // OR #
    [0xF7] = 32,  // RST 30H
    [0xF8] = 12,  // LDHL SP,n  [LD HL,SP+n]
    [0xF9] = 8,   // LD SP,HL
    [0xFA] = 16,  // LD A,(nn)
    [0xFB] = 4,   // EI
    [0xFC] = 0,   // UNUSED
    [0xFD] = 0,   // UNUSED
    [0xFE] = 8,   // CP #
    [0xFF] = 32,  // RST 38H
};

// ---------------------------------------------------------------------------
// CB-prefixed opcode cycle counts
//
// Pattern (all sourced from GB CPU Manual v1.01):
//   Register operands (B,C,D,E,H,L,A) : 8 cycles
//   (HL) operand                       : 16 cycles
//
// CB opcode low nibble layout per row:
//   x0=B  x1=C  x2=D  x3=E  x4=H  x5=L  x6=(HL)  x7=A
//   x8=B  x9=C  xA=D  xB=E  xC=H  xD=L  xE=(HL)  xF=A
//
// Instruction groups (each covers 8 opcodes, repeated x8 for bit 0-7):
//   0x00-0x07  RLC   r / (HL)
//   0x08-0x0F  RRC   r / (HL)
//   0x10-0x17  RL    r / (HL)
//   0x18-0x1F  RR    r / (HL)
//   0x20-0x27  SLA   r / (HL)
//   0x28-0x2F  SRA   r / (HL)
//   0x30-0x37  SWAP  r / (HL)
//   0x38-0x3F  SRL   r / (HL)
//   0x40-0x7F  BIT   b,r / (HL)   (8 bits x 8 regs)
//   0x80-0xBF  RES   b,r / (HL)   (8 bits x 8 regs)
//   0xC0-0xFF  SET   b,r / (HL)   (8 bits x 8 regs)
// ---------------------------------------------------------------------------
const uint8_t cb_opcode_cycles[256] = {
    // RLC r / RLC (HL)
    [0x00] = 8,   // RLC B
    [0x01] = 8,   // RLC C
    [0x02] = 8,   // RLC D
    [0x03] = 8,   // RLC E
    [0x04] = 8,   // RLC H
    [0x05] = 8,   // RLC L
    [0x06] = 16,  // RLC (HL)
    [0x07] = 8,   // RLC A

    // RRC r / RRC (HL)
    [0x08] = 8,   // RRC B
    [0x09] = 8,   // RRC C
    [0x0A] = 8,   // RRC D
    [0x0B] = 8,   // RRC E
    [0x0C] = 8,   // RRC H
    [0x0D] = 8,   // RRC L
    [0x0E] = 16,  // RRC (HL)
    [0x0F] = 8,   // RRC A

    // RL r / RL (HL)
    [0x10] = 8,   // RL B
    [0x11] = 8,   // RL C
    [0x12] = 8,   // RL D
    [0x13] = 8,   // RL E
    [0x14] = 8,   // RL H
    [0x15] = 8,   // RL L
    [0x16] = 16,  // RL (HL)
    [0x17] = 8,   // RL A

    // RR r / RR (HL)
    [0x18] = 8,   // RR B
    [0x19] = 8,   // RR C
    [0x1A] = 8,   // RR D
    [0x1B] = 8,   // RR E
    [0x1C] = 8,   // RR H
    [0x1D] = 8,   // RR L
    [0x1E] = 16,  // RR (HL)
    [0x1F] = 8,   // RR A

    // SLA r / SLA (HL)
    [0x20] = 8,   // SLA B
    [0x21] = 8,   // SLA C
    [0x22] = 8,   // SLA D
    [0x23] = 8,   // SLA E
    [0x24] = 8,   // SLA H
    [0x25] = 8,   // SLA L
    [0x26] = 16,  // SLA (HL)
    [0x27] = 8,   // SLA A

    // SRA r / SRA (HL)
    [0x28] = 8,   // SRA B
    [0x29] = 8,   // SRA C
    [0x2A] = 8,   // SRA D
    [0x2B] = 8,   // SRA E
    [0x2C] = 8,   // SRA H
    [0x2D] = 8,   // SRA L
    [0x2E] = 16,  // SRA (HL)
    [0x2F] = 8,   // SRA A

    // SWAP r / SWAP (HL)
    [0x30] = 8,   // SWAP B
    [0x31] = 8,   // SWAP C
    [0x32] = 8,   // SWAP D
    [0x33] = 8,   // SWAP E
    [0x34] = 8,   // SWAP H
    [0x35] = 8,   // SWAP L
    [0x36] = 16,  // SWAP (HL)
    [0x37] = 8,   // SWAP A

    // SRL r / SRL (HL)
    [0x38] = 8,   // SRL B
    [0x39] = 8,   // SRL C
    [0x3A] = 8,   // SRL D
    [0x3B] = 8,   // SRL E
    [0x3C] = 8,   // SRL H
    [0x3D] = 8,   // SRL L
    [0x3E] = 16,  // SRL (HL)
    [0x3F] = 8,   // SRL A

    // BIT 0,r / BIT 0,(HL)
    [0x40] = 8,   // BIT 0,B
    [0x41] = 8,   // BIT 0,C
    [0x42] = 8,   // BIT 0,D
    [0x43] = 8,   // BIT 0,E
    [0x44] = 8,   // BIT 0,H
    [0x45] = 8,   // BIT 0,L
    [0x46] = 16,  // BIT 0,(HL)
    [0x47] = 8,   // BIT 0,A

    // BIT 1,r / BIT 1,(HL)
    [0x48] = 8,   // BIT 1,B
    [0x49] = 8,   // BIT 1,C
    [0x4A] = 8,   // BIT 1,D
    [0x4B] = 8,   // BIT 1,E
    [0x4C] = 8,   // BIT 1,H
    [0x4D] = 8,   // BIT 1,L
    [0x4E] = 16,  // BIT 1,(HL)
    [0x4F] = 8,   // BIT 1,A

    // BIT 2,r / BIT 2,(HL)
    [0x50] = 8,   // BIT 2,B
    [0x51] = 8,   // BIT 2,C
    [0x52] = 8,   // BIT 2,D
    [0x53] = 8,   // BIT 2,E
    [0x54] = 8,   // BIT 2,H
    [0x55] = 8,   // BIT 2,L
    [0x56] = 16,  // BIT 2,(HL)
    [0x57] = 8,   // BIT 2,A

    // BIT 3,r / BIT 3,(HL)
    [0x58] = 8,   // BIT 3,B
    [0x59] = 8,   // BIT 3,C
    [0x5A] = 8,   // BIT 3,D
    [0x5B] = 8,   // BIT 3,E
    [0x5C] = 8,   // BIT 3,H
    [0x5D] = 8,   // BIT 3,L
    [0x5E] = 16,  // BIT 3,(HL)
    [0x5F] = 8,   // BIT 3,A

    // BIT 4,r / BIT 4,(HL)
    [0x60] = 8,   // BIT 4,B
    [0x61] = 8,   // BIT 4,C
    [0x62] = 8,   // BIT 4,D
    [0x63] = 8,   // BIT 4,E
    [0x64] = 8,   // BIT 4,H
    [0x65] = 8,   // BIT 4,L
    [0x66] = 16,  // BIT 4,(HL)
    [0x67] = 8,   // BIT 4,A

    // BIT 5,r / BIT 5,(HL)
    [0x68] = 8,   // BIT 5,B
    [0x69] = 8,   // BIT 5,C
    [0x6A] = 8,   // BIT 5,D
    [0x6B] = 8,   // BIT 5,E
    [0x6C] = 8,   // BIT 5,H
    [0x6D] = 8,   // BIT 5,L
    [0x6E] = 16,  // BIT 5,(HL)
    [0x6F] = 8,   // BIT 5,A

    // BIT 6,r / BIT 6,(HL)
    [0x70] = 8,   // BIT 6,B
    [0x71] = 8,   // BIT 6,C
    [0x72] = 8,   // BIT 6,D
    [0x73] = 8,   // BIT 6,E
    [0x74] = 8,   // BIT 6,H
    [0x75] = 8,   // BIT 6,L
    [0x76] = 16,  // BIT 6,(HL)
    [0x77] = 8,   // BIT 6,A

    // BIT 7,r / BIT 7,(HL)
    [0x78] = 8,   // BIT 7,B
    [0x79] = 8,   // BIT 7,C
    [0x7A] = 8,   // BIT 7,D
    [0x7B] = 8,   // BIT 7,E
    [0x7C] = 8,   // BIT 7,H
    [0x7D] = 8,   // BIT 7,L
    [0x7E] = 16,  // BIT 7,(HL)
    [0x7F] = 8,   // BIT 7,A

    // RES 0,r / RES 0,(HL)
    [0x80] = 8,   // RES 0,B
    [0x81] = 8,   // RES 0,C
    [0x82] = 8,   // RES 0,D
    [0x83] = 8,   // RES 0,E
    [0x84] = 8,   // RES 0,H
    [0x85] = 8,   // RES 0,L
    [0x86] = 16,  // RES 0,(HL)
    [0x87] = 8,   // RES 0,A

    // RES 1,r / RES 1,(HL)
    [0x88] = 8,   // RES 1,B
    [0x89] = 8,   // RES 1,C
    [0x8A] = 8,   // RES 1,D
    [0x8B] = 8,   // RES 1,E
    [0x8C] = 8,   // RES 1,H
    [0x8D] = 8,   // RES 1,L
    [0x8E] = 16,  // RES 1,(HL)
    [0x8F] = 8,   // RES 1,A

    // RES 2,r / RES 2,(HL)
    [0x90] = 8,   // RES 2,B
    [0x91] = 8,   // RES 2,C
    [0x92] = 8,   // RES 2,D
    [0x93] = 8,   // RES 2,E
    [0x94] = 8,   // RES 2,H
    [0x95] = 8,   // RES 2,L
    [0x96] = 16,  // RES 2,(HL)
    [0x97] = 8,   // RES 2,A

    // RES 3,r / RES 3,(HL)
    [0x98] = 8,   // RES 3,B
    [0x99] = 8,   // RES 3,C
    [0x9A] = 8,   // RES 3,D
    [0x9B] = 8,   // RES 3,E
    [0x9C] = 8,   // RES 3,H
    [0x9D] = 8,   // RES 3,L
    [0x9E] = 16,  // RES 3,(HL)
    [0x9F] = 8,   // RES 3,A

    // RES 4,r / RES 4,(HL)
    [0xA0] = 8,   // RES 4,B
    [0xA1] = 8,   // RES 4,C
    [0xA2] = 8,   // RES 4,D
    [0xA3] = 8,   // RES 4,E
    [0xA4] = 8,   // RES 4,H
    [0xA5] = 8,   // RES 4,L
    [0xA6] = 16,  // RES 4,(HL)
    [0xA7] = 8,   // RES 4,A

    // RES 5,r / RES 5,(HL)
    [0xA8] = 8,   // RES 5,B
    [0xA9] = 8,   // RES 5,C
    [0xAA] = 8,   // RES 5,D
    [0xAB] = 8,   // RES 5,E
    [0xAC] = 8,   // RES 5,H
    [0xAD] = 8,   // RES 5,L
    [0xAE] = 16,  // RES 5,(HL)
    [0xAF] = 8,   // RES 5,A

    // RES 6,r / RES 6,(HL)
    [0xB0] = 8,   // RES 6,B
    [0xB1] = 8,   // RES 6,C
    [0xB2] = 8,   // RES 6,D
    [0xB3] = 8,   // RES 6,E
    [0xB4] = 8,   // RES 6,H
    [0xB5] = 8,   // RES 6,L
    [0xB6] = 16,  // RES 6,(HL)
    [0xB7] = 8,   // RES 6,A

    // RES 7,r / RES 7,(HL)
    [0xB8] = 8,   // RES 7,B
    [0xB9] = 8,   // RES 7,C
    [0xBA] = 8,   // RES 7,D
    [0xBB] = 8,   // RES 7,E
    [0xBC] = 8,   // RES 7,H
    [0xBD] = 8,   // RES 7,L
    [0xBE] = 16,  // RES 7,(HL)
    [0xBF] = 8,   // RES 7,A

    // SET 0,r / SET 0,(HL)
    [0xC0] = 8,   // SET 0,B
    [0xC1] = 8,   // SET 0,C
    [0xC2] = 8,   // SET 0,D
    [0xC3] = 8,   // SET 0,E
    [0xC4] = 8,   // SET 0,H
    [0xC5] = 8,   // SET 0,L
    [0xC6] = 16,  // SET 0,(HL)
    [0xC7] = 8,   // SET 0,A

    // SET 1,r / SET 1,(HL)
    [0xC8] = 8,   // SET 1,B
    [0xC9] = 8,   // SET 1,C
    [0xCA] = 8,   // SET 1,D
    [0xCB] = 8,   // SET 1,E
    [0xCC] = 8,   // SET 1,H
    [0xCD] = 8,   // SET 1,L
    [0xCE] = 16,  // SET 1,(HL)
    [0xCF] = 8,   // SET 1,A

    // SET 2,r / SET 2,(HL)
    [0xD0] = 8,   // SET 2,B
    [0xD1] = 8,   // SET 2,C
    [0xD2] = 8,   // SET 2,D
    [0xD3] = 8,   // SET 2,E
    [0xD4] = 8,   // SET 2,H
    [0xD5] = 8,   // SET 2,L
    [0xD6] = 16,  // SET 2,(HL)
    [0xD7] = 8,   // SET 2,A

    // SET 3,r / SET 3,(HL)
    [0xD8] = 8,   // SET 3,B
    [0xD9] = 8,   // SET 3,C
    [0xDA] = 8,   // SET 3,D
    [0xDB] = 8,   // SET 3,E
    [0xDC] = 8,   // SET 3,H
    [0xDD] = 8,   // SET 3,L
    [0xDE] = 16,  // SET 3,(HL)
    [0xDF] = 8,   // SET 3,A

    // SET 4,r / SET 4,(HL)
    [0xE0] = 8,   // SET 4,B
    [0xE1] = 8,   // SET 4,C
    [0xE2] = 8,   // SET 4,D
    [0xE3] = 8,   // SET 4,E
    [0xE4] = 8,   // SET 4,H
    [0xE5] = 8,   // SET 4,L
    [0xE6] = 16,  // SET 4,(HL)
    [0xE7] = 8,   // SET 4,A

    // SET 5,r / SET 5,(HL)
    [0xE8] = 8,   // SET 5,B
    [0xE9] = 8,   // SET 5,C
    [0xEA] = 8,   // SET 5,D
    [0xEB] = 8,   // SET 5,E
    [0xEC] = 8,   // SET 5,H
    [0xED] = 8,   // SET 5,L
    [0xEE] = 16,  // SET 5,(HL)
    [0xEF] = 8,   // SET 5,A

    // SET 6,r / SET 6,(HL)
    [0xF0] = 8,   // SET 6,B
    [0xF1] = 8,   // SET 6,C
    [0xF2] = 8,   // SET 6,D
    [0xF3] = 8,   // SET 6,E
    [0xF4] = 8,   // SET 6,H
    [0xF5] = 8,   // SET 6,L
    [0xF6] = 16,  // SET 6,(HL)
    [0xF7] = 8,   // SET 6,A

    // SET 7,r / SET 7,(HL)
    [0xF8] = 8,   // SET 7,B
    [0xF9] = 8,   // SET 7,C
    [0xFA] = 8,   // SET 7,D
    [0xFB] = 8,   // SET 7,E
    [0xFC] = 8,   // SET 7,H
    [0xFD] = 8,   // SET 7,L
    [0xFE] = 16,  // SET 7,(HL)
    [0xFF] = 8,   // SET 7,A
};

// ---------------------------------------------------------------------------
// Not-taken cycle counts for conditional instructions.
// For unconditional or non-branching opcodes, value matches opcode_cycles[].
// Only the conditional entries differ:
//   JR cc,*   taken=12  not-taken=8
//   JP cc,nn  taken=16  not-taken=12
//   CALL cc   taken=24  not-taken=12
//   RET cc    taken=20  not-taken=8
//
// Usage example:
//   uint8_t cycles = condition_met ? opcode_cycles[op] : branch_cycles[op];
// ---------------------------------------------------------------------------
const uint8_t branch_cycles[256] = {
    [0x20] = 8,   // JR NZ,*  not-taken
    [0x28] = 8,   // JR Z,*   not-taken
    [0x30] = 8,   // JR NC,*  not-taken
    [0x38] = 8,   // JR C,*   not-taken
    [0xC0] = 8,   // RET NZ   not-taken
    [0xC2] = 12,  // JP NZ,nn not-taken
    [0xC4] = 12,  // CALL NZ  not-taken
    [0xC8] = 8,   // RET Z    not-taken
    [0xCA] = 12,  // JP Z,nn  not-taken
    [0xCC] = 12,  // CALL Z   not-taken
    [0xD0] = 8,   // RET NC   not-taken
    [0xD2] = 12,  // JP NC,nn not-taken
    [0xD4] = 12,  // CALL NC  not-taken
    [0xD8] = 8,   // RET C    not-taken
    [0xDA] = 12,  // JP C,nn  not-taken
    [0xDC] = 12,  // CALL C   not-taken
};