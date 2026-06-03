with open("test_rom.gb", "wb") as f:
    rom = bytearray(0x150)  # 336 bytes, safely fits header + your code

    # Minimal valid GB header (at 0x0100 must be the start of actual code)
    # Fill from 0x0100 onward with your test code
    rom[0x0100:0x0100 + 7] = [
        0x3E, 0x42,  # LD A, 0x42
        0x06, 0x12,  # LD B, 0x12
        0x0E, 0x34,  # LD C, 0x34
        0x76         # HALT
    ]

    f.write(rom)
