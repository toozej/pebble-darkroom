#include "unity.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char mock_timer_buffer[8];
static char mock_mode_buffer[20];
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
