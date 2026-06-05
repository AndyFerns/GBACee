#ifndef CYCLES_H
#define CYCLES_H

#include <stdint.h>

// Base opcode cycles (clock cycles).
// For conditional instructions, stores the TAKEN (longer) value.
extern const uint8_t opcode_cycles[256];

// CB-prefixed opcode cycles (clock cycles).
// Register operands = 8, (HL) operand = 16.
extern const uint8_t cb_opcode_cycles[256];

// Not-taken cycle counts for conditional branch instructions.
// Only differs from opcode_cycles[] for: JR cc, JP cc, CALL cc, RET cc.
// All other entries are 0 (unused — fall back to opcode_cycles[]).
//
// Usage:
//   uint8_t cycles = condition_met ? opcode_cycles[op] : branch_cycles[op];
extern const uint8_t branch_cycles[256];

#endif