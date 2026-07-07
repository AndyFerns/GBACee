# GBCee (A gameboy emulator in C)

GBCee is a work-in-progress Gameboy (DMG-01) emulator written from scratch in C.
This project is a deep dive into low-level systems programming, computer architecture, and the intricacies of early handheld gaming hardware.
The primary goal is to create a cycle-accurate and well-documented emulator for fun and learning.

## Current State: Debug Output

Current State: Tetris is Running!

After nearly a year of development, GBCee has reached a major milestone — it can now successfully boot and run Tetris end-to-end. This required a fully operational CPU core, PPU pipeline, interrupt system, timer subsystem, MMU, and cartridge support all working in concert.

The emulator renders the full 160×144 pixel Game Boy framebuffer to an SDL2 window, correctly handles VBlank and LCD STAT interrupts, and executes the Sharp LR35902 instruction set with enough accuracy to run commercial ROM software.

A runtime diagnostics and tracing subsystem is also in place, which logs a detailed snapshot of the CPU state for each instruction — invaluable for debugging and cross-referencing against known-good emulator traces.

```powershell
# Example of the current debug output
AF: 01B0  BC: 0013  DE: 00D8  HL: 014D  SP: FFFE  PC: 0100 | Opcode: 00 (NOP)
AF: 01B0  BC: 0013  DE: 00D8  HL: 014D  SP: FFFE  PC: 0101 | Opcode: 21 (LD HL,d16) | Operand: 014D
AF: 01B0  BC: 0013  DE: 00D8  HL: 014D  SP: FFFC  PC: 0104 | Opcode: 32 (LD (HL-),A)
AF: F1B0  BC: 0013  DE: 00D8  HL: 014C  SP: FFFC  PC: 0105 | Opcode: CB 7C (BIT 7,H)
...
```

---

## ✅ Implemented Features

The emulator has a solid foundation with several core components already in place.

* **Complete CPU Core:**
  * Full implementation of the ~500 standard and CB-prefixed Gameboy opcodes.
  * Correctly manages all 8-bit and 16-bit registers.
  * Handles the fetch-decode-execute cycle with proper program counter management.

* **Memory Management Unit (MMU):**
  * Maps the entire Gameboy memory layout (VRAM, WRAM, OAM, IO, etc.).
  * Abstracted memory access via `mmu_read()` and `mmu_write()` functions.

* **Cartridge Support:**
  * Loads `.gb` ROM files directly into memory.
  * **MBC0** (No banking) support for simple games like Tetris.
  * **MBC1** support, enabling bank switching for more complex games.

* **Debugging & Display:**
  * Real-time disassembly and register logging to the console.
  * Basic window scaffolding using the **SDL2** library, ready for PPU integration.

---

## 🗺️ Project Roadmap

This is the roadmap to a fully functional emulator. Items will be checked off as they are implemented.

### Graphics (PPU)

* [x] Implement the PPU state machine (OAM Scan, Drawing, HBlank, VBlank).
* [x] Render background and window tiles from VRAM.
* [x] Render sprites (OAM).
* [x] Draw the final 160x144 pixel buffer to the SDL window.
* [x] Handle VBlank interrupts correctly.

### Timers & Interrupts

* [x] Implement DIV, TIMA, TMA, and TAC timer registers.
* [x] Implement the Interrupt Master Enable (IME) flag and handler.
* [x] Fire interrupts correctly for VBlank, LCD STAT, Timer, etc.

### Input Handling

* [ ] Map keyboard keys to the Gameboy's button inputs (A, B, Start, Select, D-Pad).
* [ ] Write button press states to the JOYP register (0xFF00).

### Emulator Quality-of-Life Features

* [ ] **Save States:** Serialise and restore complete emulator state to/from disk.
* [ ] **Fast-forward / Rewind:** Frame-skip for fast-forward; circular frame buffer for rewind.

### Advanced Features

* [ ] **Sound (APU):** Emulate the four sound channels.
* [ ] **Additional MBCs:** Add support for MBC2, MBC3 (with RTC), and MBC5.
* [ ] **BIOS Emulation:** Load and execute the original DMG BIOS ROM.
* [ ] **Testing:** Pass Blargg's instruction and memory timing test ROMs.
* [ ] **Additional MBCs:** Add support for MBC2, MBC3 (with RTC), and MBC5.
* [ ] **BIOS Emulation:** Load and execute the original DMG boot ROM.
* [ ] **Testing:** Pass Blargg's `cpu_instrs`, `instr_timing`, and `mem_timing` test ROMs.
* [ ] **Accuracy & Performance Optimisations:** Improve cycle-level timing accuracy and rendering performance.

---

## 🛠️ How to Build and Run

### Dependencies

You will need the **SDL2** development library to compile the project.

* **On Debian/Ubuntu:**

    ```bash
    sudo apt-get install libsdl2-dev
    ```

* **On macOS (using Homebrew):**

    ```bash
    brew install sdl2
    ```

* **On Windows:**

    The recommended approach (and the one i use) is [MSYS2](https://www.msys2.org/), which provides a Unix-like shell environment and the MinGW-w64 toolchain on Windows.

    1. Download and install MSYS2 from [msys2.org](https://www.msys2.org/).

    2. Open the **MSYS2 MINGW64** shell and update the package database:

    ```bash
    pacman -Syu
    ```

    3. Install the MinGW-w64 GCC toolchain and SDL2 development libraries:

    ```bash
    pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 make
    ```

    4. After building, copy `SDL2.dll` from `C:\msys64\mingw64\bin\` into the same directory as your compiled `gbcee.exe` so it can be found at runtime. Alternatively, add `C:\msys64\mingw64\bin` to your system `PATH`.

> **Alternative (without MSYS2):** Download the SDL2 MinGW development libraries directly from the [SDL2 releases page](https://github.com/libsdl-org/SDL/releases), extract them, and point your compiler at the `include` and `lib` directories manually. This requires more manual setup of include/library paths in your build script.

### Compilation and Execution

#### Windows

1. **Clone the repository** (in your MSYS2 MINGW64 shell or Git Bash):

```bash
    git clone https://github.com/AndyFerns/GBCee.git
    cd GBCee
```
 
2. **Compile using the provided build script:**

```bash
    ./build.bat
```
 
3. **Run the emulator with a Gameboy ROM:**

```bash
    ./gbcee.exe /path/to/your/rom.gb
```

#### Linux / macOS

1. **Clone the repository:**

```bash
    git clone https://github.com/AndyFerns/GBCee.git
    cd GBCee
```

1. **Compile the project:**

```bash
    ./build
```

1. **Run the emulator with a Gameboy ROM:**

```bash
    ./gbcee /path/to/your/rom.gb
```

---

## References

* Nintendo Game Boy Programming Manual
* [Pan Docs](./documents/GBCPUManual.pdf) — Game Boy Technical Reference

## Author

Andrew Fernandes :)
