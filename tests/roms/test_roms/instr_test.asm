; instr_test.asm - Minimal test ROM for all implemented opcodes (except jumps/calls/rets)
; Assemble with: rgbasm -o instr_test.o instr_test.asm
;                rgblink -o instr_test.gb instr_test.o
;                rgbfix -v -p 0 instr_test.gb

SECTION "Header", ROM0[$100]
    jp start

SECTION "Main", ROM0[$150]
start:
    nop

    ; 8-bit loads
    ld a, $12
    ld b, a
    ld c, b
    ld d, c
    ld e, d
    ld h, e
    ld l, h

    ; 16-bit loads
    ld bc, $1234
    ld de, $5678
    ld hl, $9abc
    ld sp, $fff0

    ; 8-bit stores
    ld [hl], a
    ld a, [hl]

    ; 16-bit stores
    ld [de], a
    ld [bc], a
    ld a, [de]
    ld a, [bc]

    ; Arithmetic
    add a, b
    adc a, c
    sub d
    sbc a, e
    and a
    or a
    xor a
    cp a

    inc a
    dec a
    inc b
    dec b

    ; DAA / CPL
    daa
    cpl

    ; Stack ops
    push af
    push bc
    pop af
    pop bc

    ; Rotate/Shift
    rlca
    rrca
    rla
    rra

    ; CB-prefixed: rotate/shift
    rl c
    rr b
    sla c
    srl d

    ; CB-prefixed: bit test/set/reset
    bit 0, a
    set 0, a
    res 0, a

    ; HALT (to freeze and observe state)
    halt

loop:
    jr loop
