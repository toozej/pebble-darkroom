#include "unity.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mock color definitions for testing
typedef enum {
    TestColorBlack = 0,
    TestColorWhite = 1
} TestColor;

// Display theme structure for testing
typedef struct {
    bool is_light_background;
    TestColor text_color;
    TestColor background_color;
} DisplayTheme;

static char mock_timer_buffer[8];
static char mock_mode_buffer[32];
static char mock_timer_name_buffer[10];
static int active_timer = 1;

static TimerState* get_active_timer(void) {
    return active_timer == 1 ? get_timer1() : get_timer2();
}

static void update_timer_text(void) {
    TimerState *timer = get_active_timer();
    int minutes = timer->seconds_remaining / 60;
    int seconds = timer->seconds_remaining % 60;
    snprintf(mock_timer_buffer, sizeof(mock_timer_buffer), "%02d:%02d", minutes, seconds);
}

static void update_mode_text(void) {
    TimerState *timer = get_active_timer();
    char mode_char = (timer->mode == MODE_FILM) ? 'F' : 'P';
    const char *stage_text = "Unknown";
    switch (timer->stage) {
        case STAGE_DEVELOP: stage_text = "Dev"; break;
        case STAGE_STOP: stage_text = "Stop"; break;
        case STAGE_FIX: stage_text = "Fix"; break;
        case STAGE_WASH: stage_text = "Wash"; break;
    }
    snprintf(mock_mode_buffer, sizeof(mock_mode_buffer), "%c | %s | %s", 
             mode_char, stage_text, 
             timer->paused ? "PAUSED" : "");
}

static void update_timer_name_text(void) {
    snprintf(mock_timer_name_buffer, sizeof(mock_timer_name_buffer), "Timer %d", active_timer);
}

// Helper function to get display theme for testing
static DisplayTheme get_display_theme(int timer_number) {
    Settings *settings = get_settings();
    bool should_invert = (timer_number == 1) ? 
        settings->invert_timer1_colors : settings->invert_timer2_colors;
    
    // Timer 1 defaults to light mode, Timer 2 defaults to dark mode
    bool default_light = (timer_number == 1);
    
    // Apply inversion settings to override defaults
    if (should_invert) {
        default_light = !default_light;
    }
    
    return (DisplayTheme){
        .is_light_background = default_light,
        .text_color = default_light ? TestColorBlack : TestColorWhite,
        .background_color = default_light ? TestColorWhite : TestColorBlack
    };
}

// Enhanced mode text update with paper type support
static void update_mode_text_enhanced(void) {
    TimerState *timer = get_active_timer();
    char mode_char = (timer->mode == MODE_FILM) ? 'F' : 'P';
    
    // Get paper type string
    const char *paper_type = "Film";
    if (timer->mode == MODE_PRINT) {
        paper_type = (timer->paper_type == PAPER_RC) ? "RC" : "FB";
    }
    
    // Get stage text including new fiber stages
    const char *stage_text = "Unknown";
    switch (timer->stage) {
        case STAGE_DEVELOP: stage_text = "Dev"; break;
        case STAGE_STOP: stage_text = "Stop"; break;
        case STAGE_FIX: stage_text = "Fix"; break;
        case STAGE_WASH: stage_text = "Wash"; break;
        case STAGE_WASH1: stage_text = "Wash1"; break;
        case STAGE_HYPO_CLEAR: stage_text = "HC"; break;
        case STAGE_WASH2: stage_text = "Wash2"; break;
    }
    
    // Format: [P,F] | [RC,FB] | [Stage] | [Status]
    const char *status = timer->paused ? "PAUSED" : (timer->running ? "RUNNING" : "");
    if (strlen(status) > 0) {
        snprintf(mock_mode_buffer, sizeof(mock_mode_buffer), "%c | %s | %s | %s", 
                 mode_char, paper_type, stage_text, status);
    } else {
        snprintf(mock_mode_buffer, sizeof(mock_mode_buffer), "%c | %s | %s", 
                 mode_char, paper_type, stage_text);
    }
}

void test_display(void) {
    TimerState *timer1 = get_timer1();
    TimerState *timer2 = get_timer2();
    
    // Test 1: Test timer text formatting for different times
    timer1->seconds_remaining = 125;  // 2:05
    active_timer = 1;
    update_timer_text();
    TEST_ASSERT_EQUAL_STRING(mock_timer_buffer, "02:05");
    
    // Test 2: Test timer text for 0 seconds
    timer1->seconds_remaining = 0;
    update_timer_text();
    TEST_ASSERT_EQUAL_STRING(mock_timer_buffer, "00:00");
    
    // Test 3: Test timer text for 1 minute 30 seconds
    timer1->seconds_remaining = 90;
    update_timer_text();
    TEST_ASSERT_EQUAL_STRING(mock_timer_buffer, "01:30");
    
    // Test 4: Test mode text formatting
    timer1->mode = MODE_FILM;
    timer1->stage = STAGE_DEVELOP;
    timer1->paused = false;
    update_mode_text();
    TEST_ASSERT_EQUAL_STRING(mock_mode_buffer, "F | Dev | ");
    
    // Test 5: Test mode text with pause
    timer1->paused = true;
    update_mode_text();
    TEST_ASSERT_EQUAL_STRING(mock_mode_buffer, "F | Dev | PAUSED");
    
    // Test 6: Test mode text for different stages
    timer1->stage = STAGE_STOP;
    timer1->paused = false;
    update_mode_text();
    TEST_ASSERT_EQUAL_STRING(mock_mode_buffer, "F | Stop | ");
    
    timer1->stage = STAGE_FIX;
    update_mode_text();
    TEST_ASSERT_EQUAL_STRING(mock_mode_buffer, "F | Fix | ");
    
    timer1->stage = STAGE_WASH;
    update_mode_text();
    TEST_ASSERT_EQUAL_STRING(mock_mode_buffer, "F | Wash | ");
    
    // Test 7: Test mode text for print mode
    timer1->mode = MODE_PRINT;
    timer1->stage = STAGE_DEVELOP;
    update_mode_text();
    TEST_ASSERT_EQUAL_STRING(mock_mode_buffer, "P | Dev | ");
    
    // Test 8: Test timer name text
    active_timer = 1;
    update_timer_name_text();
    TEST_ASSERT_EQUAL_STRING(mock_timer_name_buffer, "Timer 1");
    
    active_timer = 2;
    update_timer_name_text();
    TEST_ASSERT_EQUAL_STRING(mock_timer_name_buffer, "Timer 2");
    
    // Test 9: Test switching between timers
    active_timer = 1;
    timer1->seconds_remaining = 300;
    timer2->seconds_remaining = 60;
    
    update_timer_text();
    TEST_ASSERT_EQUAL_STRING(mock_timer_buffer, "05:00");
    
    active_timer = 2;
    update_timer_text();
    TEST_ASSERT_EQUAL_STRING(mock_timer_buffer, "01:00");
    
    // Test 10: Test mode text with different timer active
    active_timer = 2;
    timer2->mode = MODE_FILM;
    timer2->stage = STAGE_STOP;
    update_mode_text();
    TEST_ASSERT_EQUAL_STRING(mock_mode_buffer, "F | Stop | ");
    
    // Test 11: Test buffer overflow protection
    timer1->seconds_remaining = 999999;  // Very large number
    active_timer = 1;
    update_timer_text();
    TEST_ASSERT_TRUE(strlen(mock_timer_buffer) < 8);
    
    // Test 12: Test invalid stage handling
    timer1->stage = 99;  // Invalid stage
    timer1->mode = MODE_FILM;
    update_mode_text();
    TEST_ASSERT_EQUAL_STRING(mock_mode_buffer, "F | Unknown | ");
    
    // Test 13: Test timer name buffer limits
    active_timer = 999;  // Large timer number
    update_timer_name_text();
    TEST_ASSERT_TRUE(strlen(mock_timer_name_buffer) > 0);
    
    printf("All display tests passed!\n");
    printf("Sample timer display: %s\n", mock_timer_buffer);
    printf("Sample mode display: %s\n", mock_mode_buffer);
    printf("Sample timer name: %s\n", mock_timer_name_buffer);
}

void test_color_scheme(void) {
    Settings *settings = get_settings();
    
    // Test 1: Timer 1 default light mode (no inversion)
    settings->invert_timer1_colors = false;
    DisplayTheme theme1 = get_display_theme(1);
    TEST_ASSERT_TRUE(theme1.is_light_background);
    TEST_ASSERT_EQUAL_INT(TestColorBlack, theme1.text_color);
    TEST_ASSERT_EQUAL_INT(TestColorWhite, theme1.background_color);
    
    // Test 2: Timer 2 default dark mode (no inversion)
    settings->invert_timer2_colors = false;
    DisplayTheme theme2 = get_display_theme(2);
    TEST_ASSERT_FALSE(theme2.is_light_background);
    TEST_ASSERT_EQUAL_INT(TestColorWhite, theme2.text_color);
    TEST_ASSERT_EQUAL_INT(TestColorBlack, theme2.background_color);
    
    // Test 3: Timer 1 with inversion (should become dark)
    settings->invert_timer1_colors = true;
    DisplayTheme theme1_inverted = get_display_theme(1);
    TEST_ASSERT_FALSE(theme1_inverted.is_light_background);
    TEST_ASSERT_EQUAL_INT(TestColorWhite, theme1_inverted.text_color);
    TEST_ASSERT_EQUAL_INT(TestColorBlack, theme1_inverted.background_color);
    
    // Test 4: Timer 2 with inversion (should become light)
    settings->invert_timer2_colors = true;
    DisplayTheme theme2_inverted = get_display_theme(2);
    TEST_ASSERT_TRUE(theme2_inverted.is_light_background);
    TEST_ASSERT_EQUAL_INT(TestColorBlack, theme2_inverted.text_color);
    TEST_ASSERT_EQUAL_INT(TestColorWhite, theme2_inverted.background_color);
    
    // Test 5: Reset settings and verify defaults
    settings->invert_timer1_colors = false;
    settings->invert_timer2_colors = false;
    
    DisplayTheme theme1_default = get_display_theme(1);
    DisplayTheme theme2_default = get_display_theme(2);
    
    // Timer 1 should be light, Timer 2 should be dark
    TEST_ASSERT_TRUE(theme1_default.is_light_background);
    TEST_ASSERT_FALSE(theme2_default.is_light_background);
    
    printf("All color scheme tests passed!\n");
    printf("Timer 1 default: %s background\n", theme1_default.is_light_background ? "Light" : "Dark");
    printf("Timer 2 default: %s background\n", theme2_default.is_light_background ? "Light" : "Dark");
}

void test_enhanced_mode_text(void) {
    TimerState *timer1 = get_timer1();
    TimerState *timer2 = get_timer2();
    
    // Test 1: RC paper mode text
    active_timer = 1;
    timer1->mode = MODE_PRINT;
    timer1->paper_type = PAPER_RC;
    timer1->stage = STAGE_DEVELOP;
    timer1->running = false;
    timer1->paused = false;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | RC | Dev", mock_mode_buffer);
    
    // Test 2: Fiber paper mode text
    active_timer = 2;
    timer2->mode = MODE_PRINT;
    timer2->paper_type = PAPER_FIBER;
    timer2->stage = STAGE_HYPO_CLEAR;
    timer2->running = false;
    timer2->paused = false;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | FB | HC", mock_mode_buffer);
    
    // Test 3: Fiber paper with Wash1 stage
    timer2->stage = STAGE_WASH1;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | FB | Wash1", mock_mode_buffer);
    
    // Test 4: Fiber paper with Wash2 stage
    timer2->stage = STAGE_WASH2;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | FB | Wash2", mock_mode_buffer);
    
    // Test 5: Running status
    timer2->running = true;
    timer2->paused = false;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | FB | Wash2 | RUNNING", mock_mode_buffer);
    
    // Test 6: Paused status
    timer2->running = false;
    timer2->paused = true;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | FB | Wash2 | PAUSED", mock_mode_buffer);
    
    // Test 7: Film mode (should show "Film" as paper type)
    timer1->mode = MODE_FILM;
    timer1->stage = STAGE_FIX;
    timer1->running = false;
    timer1->paused = false;
    active_timer = 1;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("F | Film | Fix", mock_mode_buffer);
    
    printf("All enhanced mode text tests passed!\n");
    printf("Sample enhanced mode: %s\n", mock_mode_buffer);
}

void test_stage_name_mapping(void) {
    TimerState *timer1 = get_timer1();
    
    // Test all RC paper stages
    active_timer = 1;
    timer1->mode = MODE_PRINT;
    timer1->paper_type = PAPER_RC;
    timer1->running = false;
    timer1->paused = false;
    
    // Test STAGE_DEVELOP
    timer1->stage = STAGE_DEVELOP;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | RC | Dev", mock_mode_buffer);
    
    // Test STAGE_STOP
    timer1->stage = STAGE_STOP;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | RC | Stop", mock_mode_buffer);
    
    // Test STAGE_FIX
    timer1->stage = STAGE_FIX;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | RC | Fix", mock_mode_buffer);
    
    // Test STAGE_WASH (RC paper)
    timer1->stage = STAGE_WASH;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | RC | Wash", mock_mode_buffer);
    
    // Test all Fiber paper stages including new ones
    timer1->paper_type = PAPER_FIBER;
    
    // Test STAGE_WASH1 (Fiber specific)
    timer1->stage = STAGE_WASH1;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | FB | Wash1", mock_mode_buffer);
    
    // Test STAGE_HYPO_CLEAR (Fiber specific)
    timer1->stage = STAGE_HYPO_CLEAR;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | FB | HC", mock_mode_buffer);
    
    // Test STAGE_WASH2 (Fiber specific)
    timer1->stage = STAGE_WASH2;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | FB | Wash2", mock_mode_buffer);
    
    printf("All stage name mapping tests passed!\n");
}

void test_mode_text_format_variations(void) {
    TimerState *timer1 = get_timer1();
    
    active_timer = 1;
    timer1->mode = MODE_PRINT;
    timer1->paper_type = PAPER_RC;
    timer1->stage = STAGE_DEVELOP;
    
    // Test format without status (idle state)
    timer1->running = false;
    timer1->paused = false;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | RC | Dev", mock_mode_buffer);
    
    // Test format with RUNNING status
    timer1->running = true;
    timer1->paused = false;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | RC | Dev | RUNNING", mock_mode_buffer);
    
    // Test format with PAUSED status
    timer1->running = false;
    timer1->paused = true;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("P | RC | Dev | PAUSED", mock_mode_buffer);
    
    // Test Film mode format
    timer1->mode = MODE_FILM;
    timer1->running = false;
    timer1->paused = false;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("F | Film | Dev", mock_mode_buffer);
    
    // Test Film mode with status
    timer1->running = true;
    update_mode_text_enhanced();
    TEST_ASSERT_EQUAL_STRING("F | Film | Dev | RUNNING", mock_mode_buffer);
    
    printf("All mode text format variation tests passed!\n");
}

void test_color_theme_comprehensive(void) {
    Settings *settings = get_settings();
    
    // Test all combinations of timer defaults and inversion settings
    
    // Timer 1 scenarios
    settings->invert_timer1_colors = false;
    DisplayTheme theme1_normal = get_display_theme(1);
    TEST_ASSERT_TRUE(theme1_normal.is_light_background);
    TEST_ASSERT_EQUAL_INT(TestColorBlack, theme1_normal.text_color);
    TEST_ASSERT_EQUAL_INT(TestColorWhite, theme1_normal.background_color);
    
    settings->invert_timer1_colors = true;
    DisplayTheme theme1_inverted = get_display_theme(1);
    TEST_ASSERT_FALSE(theme1_inverted.is_light_background);
    TEST_ASSERT_EQUAL_INT(TestColorWhite, theme1_inverted.text_color);
    TEST_ASSERT_EQUAL_INT(TestColorBlack, theme1_inverted.background_color);
    
    // Timer 2 scenarios
    settings->invert_timer2_colors = false;
    DisplayTheme theme2_normal = get_display_theme(2);
    TEST_ASSERT_FALSE(theme2_normal.is_light_background);
    TEST_ASSERT_EQUAL_INT(TestColorWhite, theme2_normal.text_color);
    TEST_ASSERT_EQUAL_INT(TestColorBlack, theme2_normal.background_color);
    
    settings->invert_timer2_colors = true;
    DisplayTheme theme2_inverted = get_display_theme(2);
    TEST_ASSERT_TRUE(theme2_inverted.is_light_background);
    TEST_ASSERT_EQUAL_INT(TestColorBlack, theme2_inverted.text_color);
    TEST_ASSERT_EQUAL_INT(TestColorWhite, theme2_inverted.background_color);
    
    // Test that Timer 1 and Timer 2 have opposite defaults
    settings->invert_timer1_colors = false;
    settings->invert_timer2_colors = false;
    DisplayTheme theme1_default = get_display_theme(1);
    DisplayTheme theme2_default = get_display_theme(2);
    
    TEST_ASSERT_TRUE(theme1_default.is_light_background != theme2_default.is_light_background);
    TEST_ASSERT_TRUE(theme1_default.text_color != theme2_default.text_color);
    TEST_ASSERT_TRUE(theme1_default.background_color != theme2_default.background_color);
    
    printf("All comprehensive color theme tests passed!\n");
}
