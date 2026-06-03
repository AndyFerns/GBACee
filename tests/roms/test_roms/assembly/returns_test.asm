; Test ROM for RET, RET cc, RETI
; Assemble with: rgbasm -o ret_test.o ret_test.asm
;                rgblink -o ret_test.gb ret_test.o
;                rgbfix -v -p 0 ret_test.gb

SECTION "Header", ROM0[$100]
    jp Start

SECTION "Test", ROM0[$150]

Start:
    ld a, 1
    cp a         ; sets Z flag
    call Z, RetZTest
    call NZ, RetNZTest

    xor a
    sbc a        ; clears C flag
    call NC, RetNCTest
    scf
    call C, RetCTest

    call SimpleRetTest

    ; Setup interrupt test
    ei
    jp WaitForever

RetZTest:
    nop
    ret

RetNZTest:
    nop
    ret

RetNCTest:
    nop
    ret

RetCTest:
    nop
    ret

SimpleRetTest:
    nop
    ret

WaitForever:
    halt
    jp WaitForever

; Interrupt vector at 0x40 will use RETI
SECTION "VBlank Interrupt", ROM0[$40]
    reti
