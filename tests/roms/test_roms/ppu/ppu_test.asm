; ============================================================================
; ppu_test.asm - GBCee PPU Rendering & Features Test ROM
; ============================================================================

SECTION "Header", ROM0[$0100]
    nop
    jp Start

; Game Boy Header Area
SECTION "HeaderPad", ROM0[$0150]

SECTION "Main", ROM0[$0150]
Start:
    di                  ; Disable interrupts
    ld sp, $FFFE        ; Set up stack pointer

    ; Wait for VBlank before disabling LCD
.wait_vblank:
    ld a, [$FF44]       ; Read LY
    cp 144
    jr c, .wait_vblank

    xor a
    ld [$FF40], a       ; Turn off LCD (LCDC = 0)

    ; ----------------------------------------------------
    ; 1. Load Tile Data into VRAM ($8000-$97FF)
    ; ----------------------------------------------------
    ; Tile 0: Checkerboard pattern (for BG/Window)
    ; Tile 1: Solid border block
    ; Tile 2: Sprite pattern (hollow square shape)
    
    ld hl, TileData
    ld de, $8000
    ld bc, TileDataEnd - TileData
.copy_tiles:
    ld a, [hli]
    ld [de], a
    inc de
    dec bc
    ld a, b
    or c
    jr nz, .copy_tiles

    ; ----------------------------------------------------
    ; 2. Initialize BG Map ($9800-$9BFF) with Tile 0
    ; ----------------------------------------------------
    ld hl, $9800
    ld bc, $0400        ; 1024 bytes (32x32 tiles)
.init_bg:
    ld a, 0             ; Tile 0 (checkerboard)
    ld [hli], a
    dec bc
    ld a, b
    or c
    jr nz, .init_bg

    ; Draw a horizontal line on the BG map using Tile 1 to test scrolling alignment
    ld hl, $9800 + 32 * 5 ; Row 5
    ld d, 32
    ld a, 1             ; Tile 1 (border/solid block)
.draw_bg_line:
    ld [hli], a
    dec d
    jr nz, .draw_bg_line

    ; ----------------------------------------------------
    ; 3. Initialize Window Map ($9C00-$9FFF) with Tile 1
    ; ----------------------------------------------------
    ld hl, $9C00
    ld bc, $0400
.init_window:
    ld a, 1             ; Tile 1
    ld [hli], a
    dec bc
    ld a, b
    or c
    jr nz, .init_window

    ; Write some pattern inside the window area (Row 2, Column 2)
    ld hl, $9C00 + 32 * 2 + 2
    ld a, 0             ; Checkerboard inside window
    ld [hl], a

    ; ----------------------------------------------------
    ; 4. Initialize OAM ($FE00-$FE9F)
    ; ----------------------------------------------------
    ; Configure 4 sprites with different features to test PPU:
    ; Sprite 1: Normal (X=30, Y=40, Tile 2)
    ; Sprite 2: X-Flipped (X=60, Y=40, Tile 2, X-Flip flag)
    ; Sprite 3: Y-Flipped (X=90, Y=40, Tile 2, Y-Flip flag)
    ; Sprite 4: Palette 1 & Priority (X=120, Y=40, Tile 2, OBP1 + Behind BG flags)
    
    ; Clear all OAM first
    ld hl, $FE00
    ld c, 160
    xor a
.clear_oam:
    ld [hli], a
    dec c
    jr nz, .clear_oam

    ; Sprite 1: Normal
    ld hl, $FE00
    ld a, 40 + 16       ; Y (offset by 16)
    ld [hli], a
    ld a, 30 + 8        ; X (offset by 8)
    ld [hli], a
    ld a, 2             ; Tile ID
    ld [hli], a
    ld a, %00000000     ; Flags
    ld [hli], a

    ; Sprite 2: X-Flipped
    ld a, 40 + 16       ; Y
    ld [hli], a
    ld a, 60 + 8        ; X
    ld [hli], a
    ld a, 2             ; Tile ID
    ld [hli], a
    ld a, %00100000     ; Flags: X-Flip (bit 5)
    ld [hli], a

    ; Sprite 3: Y-Flipped
    ld a, 40 + 16       ; Y
    ld [hli], a
    ld a, 90 + 8        ; X
    ld [hli], a
    ld a, 2             ; Tile ID
    ld [hli], a
    ld a, %01000000     ; Flags: Y-Flip (bit 6)
    ld [hli], a

    ; Sprite 4: Palette 1 & Priority (Behind BG)
    ld a, 40 + 16       ; Y
    ld [hli], a
    ld a, 120 + 8       ; X
    ld [hli], a
    ld a, 2             ; Tile ID
    ld [hli], a
    ld a, %10010000     ; Flags: Priority Behind BG (bit 7), Palette 1 (bit 4)
    ld [hli], a

    ; ----------------------------------------------------
    ; 5. Set up PPU Registers
    ; ----------------------------------------------------
    ld a, %11100100     ; Palette: 11 10 01 00 (White, L.Gray, D.Gray, Black)
    ld [$FF47], a       ; BGP
    
    ld a, %11100100     ; Sprite Palette 0 (Standard)
    ld [$FF48], a       ; OBP0

    ld a, %00011011     ; Sprite Palette 1 (Inverted colors)
    ld [$FF49], a       ; OBP1

    ld a, 80            ; Window Y coordinate
    ld [$FF4A], a       ; WY

    ld a, 7             ; Window X coordinate (7 = screen left edge)
    ld [$FF4B], a       ; WX

    ld a, 5             ; Scroll Y
    ld [$FF42], a       ; SCY

    ld a, 5             ; Scroll X
    ld [$FF43], a       ; SCX

    ; Enable LCD with standard options:
    ; Bit 7: LCD Enable (1)
    ; Bit 6: Window Map Select ($9C00 = 1)
    ; Bit 5: Window Enable (1)
    ; Bit 4: BG & Window Tile Data Select ($8000 = 1)
    ; Bit 3: BG Map Select ($9800 = 0)
    ; Bit 2: OBJ Size (8x8 = 0)
    ; Bit 1: OBJ Enable (1)
    ; Bit 0: BG/Window Display Priority (1)
    ld a, %11110011     ; LCDC value
    ld [$FF40], a       ; LCDC

    ; ----------------------------------------------------
    ; 6. Main Spin Loop
    ; ----------------------------------------------------
MainLoop:
    halt
    jr MainLoop

; ----------------------------------------------------
; Tile Data Definitions (16 bytes per tile)
; ----------------------------------------------------
TileData:
    ; Tile 0: Checkerboard pattern
    db $AA, $00, $55, $00, $AA, $00, $55, $00
    db $AA, $00, $55, $00, $AA, $00, $55, $00

    ; Tile 1: Solid border block (thick borders, dark interior)
    db $FF, $FF, $81, $81, $81, $81, $81, $81
    db $81, $81, $81, $81, $81, $81, $FF, $FF

    ; Tile 2: Sprite pattern (hollow square/circle outline)
    db $3C, $3C, $42, $42, $81, $81, $81, $81
    db $81, $81, $81, $81, $42, $42, $3C, $3C
TileDataEnd:
