; Save this as test_roms/basic_test.asm

SECTION "Entry Point", ROM0[$0100]
    jp Start           ; Jump to our code (opcode 0xC3)

SECTION "Code", ROM0[$0150]
Start:
    ld b, $05          ; B = 5
    ld c, $0A          ; C = 10

    inc b              ; B = 6
    dec c              ; C = 9

    ld a, b            ; A = B (6)
    ld b, a            ; B = A (6 again, just to test register copy)

    ld a, c            ; A = C (9)
    dec b              ; B = 5
    inc c              ; C = 10

    halt               ; opcode 0x76, safely halts emulator
