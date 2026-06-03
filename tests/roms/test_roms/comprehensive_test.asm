; ============================================================================
; comprehensive_test.asm - GBCee Comprehensive Feature Test ROM
; ============================================================================
;
; Tests all currently implemented emulator features:
;   1. 8-bit load operations (LD r, n / LD r, r / LD r, (HL) / LD (HL), r)
;   2. 16-bit load operations (LD rr, nn / LD SP, HL)
;   3. 8-bit ALU operations (ADD, SUB, AND, OR, XOR, CP, INC, DEC)
;   4. 16-bit ALU operations (ADD HL, rr / INC rr / DEC rr)
;   5. Stack operations (PUSH / POP)
;   6. Rotate/Shift instructions (RLCA, RLA, RRCA, RRA)
;   7. CB-prefixed operations (RLC, SLA, SRA, SRL, BIT, SET, RES, SWAP)
;   8. Jump instructions (JP, JR, JP cc)
;   9. CALL / RET
;  10. Misc: NOP, CPL, SCF, CCF, DAA
;  11. Serial output (0xFF01 Blargg test)
;  12. Memory read/write (WRAM)
;
; Designed to be used with the --trace-cpu flag:
;   ./gbcee comprehensive_test.gb --trace-cpu
;
; Or log to file:
;   ./gbcee comprehensive_test.gb --trace-file trace.log
;
; Each section writes a "marker" value to register A to help identify
; test sections in the trace output.
;
; ============================================================================

SECTION "Header", ROM0[$100]
    nop
    jp Start

; Pad to $150 (standard GB header area)
SECTION "HeaderPad", ROM0[$150]

; ============================================================================
; Test Code
; ============================================================================

SECTION "Main", ROM0[$150]

Start:

; ---- Section 1: 8-bit Loads ----
; Marker: A = 0x01
    ld a, $01           ; Section marker

    ld b, $AB           ; LD B, n
    ld c, $CD           ; LD C, n
    ld d, $EF           ; LD D, n
    ld e, $12           ; LD E, n
    ld h, $34           ; LD H, n
    ld l, $56           ; LD L, n

    ld a, b             ; LD A, B  -> A = 0xAB
    ld b, c             ; LD B, C  -> B = 0xCD
    ld c, d             ; LD C, D  -> C = 0xEF
    ld d, e             ; LD D, E  -> D = 0x12
    ld e, h             ; LD E, H  -> E = 0x34
    ld h, l             ; LD H, L  -> H = 0x56

; ---- Section 2: 16-bit Loads ----
; Marker: A = 0x02
    ld a, $02           ; Section marker

    ld bc, $BEEF        ; LD BC, nn
    ld de, $CAFE        ; LD DE, nn
    ld hl, $DEAD        ; LD HL, nn
    ld sp, $FFFE        ; LD SP, nn
    ld sp, hl           ; LD SP, HL -> SP = 0xDEAD


; ---- Section 3: Memory Read/Write via (HL) ----
; Marker: A = 0x03
    ld a, $03           ; Section marker

    ld sp, $FFFE        ; Restore SP for later stack use
    ld hl, $C000        ; Point HL to WRAM start
    ld a, $77           ; Value to store
    ld [hl], a          ; Write 0x77 to [0xC000]
    ld b, [hl]          ; Read back -> B = 0x77


; ---- Section 4: 8-bit ALU ----
; Marker: A = 0x04
    ld a, $04           ; Section marker

    ; ADD
    ld a, $0F
    ld b, $01
    add a, b            ; A = 0x10, H flag set

    ; SUB
    ld a, $10
    ld c, $01
    sub c               ; A = 0x0F, N and H flags set

    ; AND
    ld a, $FF
    and $0F             ; A = 0x0F, H flag set

    ; OR
    ld a, $F0
    or $0F              ; A = 0xFF

    ; XOR
    ld a, $FF
    xor a               ; A = 0x00, Z flag set

    ; CP
    ld a, $42
    cp $42              ; Z flag set, N flag set

    ; INC / DEC
    ld a, $FF
    inc a               ; A = 0x00, Z and H flags set
    dec a               ; A = 0xFF, N and H flags set


; ---- Section 5: 16-bit ALU ----
; Marker: A = 0x05
    ld a, $05           ; Section marker

    ld hl, $0FFF
    ld bc, $0001
    add hl, bc          ; HL = 0x1000, H flag set

    ld bc, $1234
    inc bc              ; BC = 0x1235
    dec bc              ; BC = 0x1234

    ld de, $FFFF
    inc de              ; DE = 0x0000 (wrap)


; ---- Section 6: Stack Operations ----
; Marker: A = 0x06
    ld a, $06           ; Section marker

    ld sp, $FFFE
    ld bc, $ABCD
    push bc             ; Stack: [0xAB, 0xCD], SP = 0xFFFC
    ld bc, $0000        ; Clear BC
    pop bc              ; BC = 0xABCD, SP = 0xFFFE

    ld de, $1234
    push de             ; Stack: [0x12, 0x34], SP = 0xFFFC
    pop hl              ; HL = 0x1234, SP = 0xFFFE


; ---- Section 7: Rotates (A-register fast ops) ----
; Marker: A = 0x07
    ld a, $07           ; Section marker

    ld a, $81           ; 10000001
    rlca                ; A = 0x03, C flag set (bit 7 was 1)

    ld a, $81
    rrca                ; A = 0xC0, C flag set (bit 0 was 1)

    ld a, $80
    scf                 ; Set carry
    rla                 ; A = 0x01 (carry rotated in), C flag set

    ld a, $01
    scf                 ; Set carry
    rra                 ; A = 0x80 (carry rotated in), C flag set


; ---- Section 8: CB-Prefixed Operations ----
; Marker: A = 0x08
    ld a, $08           ; Section marker

    ; RLC B
    ld b, $80           ; 10000000
    rlc b               ; B = 0x01, C flag set

    ; SLA C
    ld c, $40           ; 01000000
    sla c               ; C = 0x80

    ; SRA D
    ld d, $81           ; 10000001
    sra d               ; D = 0xC0, C flag set (arithmetic shift preserves bit 7)

    ; SRL E
    ld e, $03           ; 00000011
    srl e               ; E = 0x01, C flag set

    ; SWAP H
    ld h, $AB           ; 10101011
    swap h              ; H = 0xBA

    ; BIT 7, A
    ld a, $80
    bit 7, a            ; Z flag clear (bit 7 is set)

    ld a, $00
    bit 7, a            ; Z flag set (bit 7 is clear)

    ; SET / RES
    ld b, $00
    set 3, b            ; B = 0x08
    res 3, b            ; B = 0x00


; ---- Section 9: Misc Instructions ----
; Marker: A = 0x09
    ld a, $09           ; Section marker

    nop                 ; No operation

    ld a, $AB
    cpl                 ; A = 0x54, N and H flags set

    scf                 ; Set carry flag
    ccf                 ; Complement carry flag (clear it)

    ; DAA test: 0x09 + 0x01 = 0x0A -> DAA corrects to 0x10
    ld a, $09
    ld b, $01
    add a, b            ; A = 0x0A
    daa                 ; A = 0x10 (BCD correction)


; ---- Section 10: Jumps ----
; Marker: A = 0x0A
    ld a, $0A           ; Section marker

    ; JR (unconditional relative jump)
    jr .jump_target_1
    ld a, $FF           ; This should be SKIPPED
.jump_target_1:

    ; JP (unconditional absolute jump)
    jp .jump_target_2
    ld a, $FF           ; This should be SKIPPED
.jump_target_2:

    ; JP NZ (conditional - should be taken since Z is clear)
    ld a, $01           ; A != 0, so Z flag is clear
    or a                ; Set flags based on A (Z clear)
    jp nz, .jump_target_3
    ld a, $FF           ; This should be SKIPPED
.jump_target_3:

    ; JP Z (conditional - should NOT be taken since Z is clear)
    ld a, $01
    or a                ; Z clear
    jp z, .jump_skip    ; Should NOT jump
    jr .jump_ok         ; This should execute
.jump_skip:
    ld a, $FF           ; FAIL if reached
.jump_ok:


; ---- Section 11: CALL / RET ----
; Marker: A = 0x0B
    ld a, $0B           ; Section marker

    ld sp, $FFFE        ; Ensure valid SP
    call TestSubroutine ; Should jump to subroutine and return
    ; After return: A = 0x42 (set by subroutine)

    jr AfterSub         ; Skip over the subroutine body

TestSubroutine:
    ld a, $42
    ret

AfterSub:


; ---- Section 12: Serial Output (Blargg) ----
; Marker: A = 0x0C
    ld a, $0C           ; Section marker

    ; Write characters to the serial port (0xFF01)
    ; This tests the --blargg / serial output feature
    ld a, $4F           ; 'O'
    ld [$FF01], a
    ld a, $4B           ; 'K'
    ld [$FF01], a
    ld a, $0A           ; newline
    ld [$FF01], a


; ---- Section 13: Final Register State ----
; Set up a known final state for verification
    ld a, $AA
    ld b, $BB
    ld c, $CC
    ld d, $DD
    ld e, $EE
    ld h, $C0
    ld l, $FE

    ; Halt the CPU
    halt
