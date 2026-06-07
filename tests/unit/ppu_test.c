#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "ppu.h"
#include "mmu.h"

extern PPU ppu;
extern mmu_t mmu;

/* Test Framework (Emoji-free) */
static int tests_run = 0;
static int tests_failed = 0;

#define TEST_CASE(name) static void test_##name(void)
#define RUN_TEST(name) do { printf("--- Running test: %s ---\n", #name); test_##name(); } while (0)

#define ASSERT_EQ(a, b, message) \
    do { \
        tests_run++; \
        if ((a) != (b)) { \
            fprintf(stderr, "    [FAIL] %s:%d: " message " - Expected 0x%X, got 0x%X\n", __FILE__, __LINE__, (int)(b), (int)(a)); \
            tests_failed++; \
        } else { \
            printf("    [PASS] %s\n", message); \
        } \
    } while (0)

#define ASSERT_TRUE(condition, message) \
    do { \
        tests_run++; \
        if (!(condition)) { \
            fprintf(stderr, "    [FAIL] %s:%d: " message "\n", __FILE__, __LINE__); \
            tests_failed++; \
        } else { \
            printf("    [PASS] %s\n", message); \
        } \
    } while (0)

/* Setup/Teardown */
static void setup(void) {
    mmu_init();
    ppu_init();
    /* Enable LCD so PPU logic runs */
    mmu_write(0xFF40, LCDC_LCD_ENABLE);
}

static void teardown(void) {
    ppu_shutdown();
    mmu_free();
}

/* Test Cases */
TEST_CASE(ppu_initialization) {
    setup();
    ASSERT_TRUE(ppu.window != NULL, "SDL window should be created");
    ASSERT_TRUE(ppu.renderer != NULL, "SDL renderer should be created");
    ASSERT_TRUE(ppu.texture != NULL, "SDL texture should be created");
    ASSERT_EQ(ppu.mode, PPU_MODE_OAM, "Initial PPU mode should be OAM (2)");
    ASSERT_EQ(ppu.dots, 0, "Initial dots should be 0");
    ASSERT_EQ(ppu.ly, 0, "Initial LY should be 0");
    teardown();
}

TEST_CASE(mode_transitions_and_timing) {
    setup();
    
    /* Initially Mode 2 (OAM). Runs for 80 dots. */
    ppu_step(79);
    ASSERT_EQ(ppu.mode, PPU_MODE_OAM, "PPU should remain in OAM mode at dot 79");
    
    ppu_step(1);
    ASSERT_EQ(ppu.mode, PPU_MODE_DRAWING, "PPU should transition to DRAWING mode at dot 80");
    
    /* Mode 3 (Drawing). Runs for 172 dots. */
    ppu_step(171);
    ASSERT_EQ(ppu.mode, PPU_MODE_DRAWING, "PPU should remain in DRAWING mode at dot 251");
    
    ppu_step(1);
    ASSERT_EQ(ppu.mode, PPU_MODE_HBLANK, "PPU should transition to HBLANK mode at dot 252");
    
    /* Mode 0 (HBlank). Runs for 204 dots. */
    ppu_step(203);
    ASSERT_EQ(ppu.mode, PPU_MODE_HBLANK, "PPU should remain in HBLANK mode at dot 455");
    ASSERT_EQ(ppu.ly, 0, "LY should be 0 at end of scanline 0");
    
    ppu_step(1);
    ASSERT_EQ(ppu.mode, PPU_MODE_OAM, "PPU should transition to OAM mode for next scanline");
    ASSERT_EQ(ppu.ly, 1, "LY should increment to 1");
    
    teardown();
}

TEST_CASE(lcdc_disabled) {
    setup();
    /* Disable LCD */
    mmu_write(0xFF40, 0x00);
    
    /* Step multiple times to ensure state is fixed */
    ppu_step(1000);
    
    ASSERT_EQ(ppu.mode, PPU_MODE_HBLANK, "Disabled PPU should be in HBLANK mode");
    ASSERT_EQ(ppu.dots, 0, "Disabled PPU should keep dots at 0");
    ASSERT_EQ(ppu.ly, 0, "Disabled PPU should keep LY at 0");
    ASSERT_EQ(mmu_read(0xFF44), 0, "Disabled PPU should keep LY register at 0");
    
    teardown();
}

TEST_CASE(lyc_coincidence) {
    setup();
    
    /* Set LYC to 2 */
    mmu_write(0xFF45, 2);
    /* Enable LYC interrupt in STAT */
    mmu_write(0xFF41, STAT_LYC_INT);
    
    /* Step through scanline 0 and 1 one cycle at a time */
    for (int i = 0; i < 456 * 2; i++) {
        ppu_step(1);
    }
    
    ASSERT_EQ(ppu.ly, 2, "LY should be 2 after 2 scanlines");
    ASSERT_TRUE((mmu_read(0xFF41) & STAT_LYC_FLAG) != 0, "STAT LYC flag should be set when LY == LYC");
    ASSERT_TRUE((mmu_read(0xFF0F) & 0x02) != 0, "LCD STAT interrupt flag should be set in IF");
    
    teardown();
}

TEST_CASE(stat_interrupts) {
    setup();
    
    /* Enable HBlank interrupt in STAT */
    mmu_write(0xFF41, STAT_HBLANK_INT);
    /* Clear IF */
    mmu_write(0xFF0F, 0x00);
    
    /* Step into Drawing mode (80 dots) */
    ppu_step(80);
    ASSERT_EQ(mmu_read(0xFF0F) & 0x02, 0, "IF should not be set yet");
    
    /* Step into HBlank mode (172 dots) */
    ppu_step(172);
    ASSERT_TRUE((mmu_read(0xFF0F) & 0x02) != 0, "HBlank STAT interrupt should fire");
    
    teardown();
}

TEST_CASE(oam_scan_logic) {
    setup();
    
    /* Set LY to 10 for testing */
    ppu.ly = 10;
    
    /* Sprite 1: Y = 20 (Visible on LY 10: 20 - 16 = 4. 10 is between 4 and 4+8) */
    mmu_write(0xFE00, 20); 
    mmu_write(0xFE01, 10);
    mmu_write(0xFE02, 0);
    mmu_write(0xFE03, 0);
    
    /* Sprite 2: Y = 40 (Not visible on LY 10) */
    mmu_write(0xFE04, 40);
    mmu_write(0xFE05, 10);
    mmu_write(0xFE06, 0);
    mmu_write(0xFE07, 0);
    
    /* Sprite 3: Y = 22 (Visible on LY 10: 22 - 16 = 6) */
    mmu_write(0xFE08, 22);
    mmu_write(0xFE09, 10);
    mmu_write(0xFE0A, 0);
    mmu_write(0xFE0B, 0);
    
    /* Trigger OAM scan explicitly via state transition from HBlank to OAM */
    ppu.mode = PPU_MODE_HBLANK; 
    ppu.dots = 204;
    ppu.ly = 9;
    ppu_step(0); /* Transitions to OAM and increments LY to 10 */
    
    /* We expect 2 sprites to be found */
    ASSERT_EQ(ppu.sprite_count, 2, "Should find exactly 2 visible sprites");
    ASSERT_EQ(ppu.sprite_buffer[0].y, 20, "First visible sprite should have Y=20");
    ASSERT_EQ(ppu.sprite_buffer[1].y, 22, "Second visible sprite should have Y=22");
    
    teardown();
}

/* Main */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    printf("Starting PPU test suite...\n\n");

    RUN_TEST(ppu_initialization);
    RUN_TEST(mode_transitions_and_timing);
    RUN_TEST(lcdc_disabled);
    RUN_TEST(lyc_coincidence);
    RUN_TEST(stat_interrupts);
    RUN_TEST(oam_scan_logic);

    printf("\n----------------------------------------\n");
    if (tests_failed == 0) {
        printf("All %d tests passed! [PASS]\n", tests_run);
    } else {
        printf("%d of %d tests failed. [FAIL]\n", tests_failed, tests_run);
    }
    printf("----------------------------------------\n");

    return tests_failed > 0;
}
