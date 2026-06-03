; alu_test.asm
SECTION "Start", ROM0[$0100]
    jp start

SECTION "Main", ROM0[$0150]
start:
    ; Initialize registers
    ld a, $10
    ld b, $01
    ld c, $02
    ld d, $03
    ld e, $04
    ld h, $12
    ld l, $34
    ld sp, $FFF0

    ; --- 8-bit ALU ops ---
    add a, b        ; A = 0x10 + 0x01 = 0x11
    adc a, c        ; A = 0x11 + 0x02 + carry
    sub d           ; A = A - D
    sbc a, e        ; A = A - E - carry
    cp b            ; Compare A with B
    inc b           ; B = 0x02
    dec c           ; C = 0x01

    ; --- Logical ops ---
    and a           ; A &= A
    xor d           ; A ^= D
    or e            ; A |= E

    ; --- Misc ops ---
    daa             ; Decimal adjust A
    cpl             ; A = ~A

    ; --- 16-bit ALU ops ---
    add hl, bc      ; HL = HL + BC
    add hl, de      ; HL = HL + DE
    add hl, hl      ; HL = HL + HL
    add hl, sp      ; HL = HL + SP

    ; --- Add SP, immediate ---
    ld a, $01
    ld [$C000], a   ; store to known location
    ld a, $FE       ; -2 in signed
    add sp, -2      ; SP = SP - 2

halt_loop:
    jp halt_loop    ; Infinite loop so you can inspect memory/registers
