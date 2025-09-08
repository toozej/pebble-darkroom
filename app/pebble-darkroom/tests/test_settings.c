#include "unity.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>

// Mock globals for testing persistence simulation
bool persist_write_called = false;
bool persist_read_called = false;
int persist_key = -1;
int persist_size = 0;

// Mock persist_write_data function

// Test group for settings
void test_settings(void) {
    // Test 1: Verify default settings values
    TEST_ASSERT_TRUE(get_settings()->vibration_enabled == true);
    TEST_ASSERT_TRUE(get_settings()->backlight_enabled == false);
    TEST_ASSERT_TRUE(get_settings()->invert_timer1_colors == false);
    TEST_ASSERT_TRUE(get_settings()->invert_timer2_colors == false);
    TEST_ASSERT_TRUE(get_settings()->invert_menu_colors == false);
    
    // Test 2: Toggle vibration setting
    Settings *settings = get_settings();
    settings->vibration_enabled = false;
    save_settings();
    TEST_ASSERT_TRUE(settings->vibration_enabled == false);
    TEST_ASSERT_TRUE(persist_write_called == true);
    TEST_ASSERT_TRUE(persist_key == SETTINGS_KEY);
    
    // Test 3: Toggle backlight setting
    settings->backlight_enabled = true;
    save_settings();
    TEST_ASSERT_TRUE(settings->backlight_enabled == true);
    
    // Test 4: Test load_settings function
    persist_read_called = false;
    load_settings();
    TEST_ASSERT_TRUE(persist_read_called == true);
    TEST_ASSERT_TRUE(persist_key == SETTINGS_KEY);
    
    // Test 5: Verify timer times arrays
    int *film = get_film_times();
    int *print = get_print_times();
    
    TEST_ASSERT_EQUAL_INT(film[STAGE_DEVELOP], 300);
    TEST_ASSERT_EQUAL_INT(film[STAGE_STOP], 60);
    TEST_ASSERT_EQUAL_INT(film[STAGE_FIX], 300);
    TEST_ASSERT_EQUAL_INT(film[STAGE_WASH], 300);
    
    TEST_ASSERT_EQUAL_INT(print[STAGE_DEVELOP], 60);
    TEST_ASSERT_EQUAL_INT(print[STAGE_STOP], 30);
    TEST_ASSERT_EQUAL_INT(print[STAGE_FIX], 300);
    TEST_ASSERT_EQUAL_INT(print[STAGE_WASH], 300);
    
    // Test 6: Test settings string representation
    char *settings_str = settings_to_string(get_settings());
    TEST_ASSERT_TRUE(strlen(settings_str) > 0);
    printf("Settings string: %s\n", settings_str);
    
    // Test 7: Test timer reset functionality
    TimerState *timer1 = get_timer1();
    timer1->stage = STAGE_FIX;
    timer1->seconds_remaining = 100;
    timer1->running = true;
    timer1->paused = true;
    timer1->mode = MODE_FILM;
    
    reset_timer(timer1);
    TEST_ASSERT_TRUE(!timer1->running);
    TEST_ASSERT_TRUE(!timer1->paused);
    TEST_ASSERT_EQUAL_INT(timer1->stage, STAGE_DEVELOP);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 300);
    
    // Test 8: Test timer pause functionality
    timer1->running = true;
    timer1->paused = false;
    timer1->timer_handle = (void*)0x12345678;
    
    pause_timer(timer1);
    TEST_ASSERT_TRUE(!timer1->running);
    TEST_ASSERT_TRUE(timer1->paused);
    TEST_ASSERT_TRUE(timer1->timer_handle == NULL);
    
    // Test 9: Test timer resume functionality
    resume_timer(timer1);
    TEST_ASSERT_TRUE(timer1->running);
    TEST_ASSERT_TRUE(!timer1->paused);
    TEST_ASSERT_TRUE(timer1->timer_handle != NULL);
    
    // Test 10: Test timer string representation
    char *timer_str = timer_to_string(timer1);
    TEST_ASSERT_TRUE(strlen(timer_str) > 0);
    printf("Timer string: %s\n", timer_str);
    
    // Test 11: Test different timer mode reset
    timer1->mode = MODE_PRINT;
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 60);
    
    printf("All settings tests passed!\n");
}

// setUp and tearDown for individual tests
void setUp(void) {
    // Reset mock flags before each test
    persist_write_called = false;
    persist_read_called = false;
    persist_key = -1;
    persist_size = 0;
    
    // Reset settings to defaults
    Settings *settings = get_settings();
    settings->vibration_enabled = true;
    settings->backlight_enabled = false;
    settings->invert_timer1_colors = false;
    settings->invert_timer2_colors = false;
    settings->invert_menu_colors = false;
    
    // Reset timer1 to defaults
    TimerState *timer1 = get_timer1();
    timer1->running = false;
    timer1->paused = false;
    timer1->mode = MODE_PRINT;
    timer1->stage = STAGE_DEVELOP;
    timer1->seconds_remaining = 60;
    timer1->timer_handle = NULL;
}