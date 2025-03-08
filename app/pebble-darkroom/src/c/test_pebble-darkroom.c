// test_pebble-darkroom.c - Unit tests for Pebble Darkroom app
#include <pebble.h>
#include "unity/unity.h"
#include "mock_pebble.h"

// Include the main app source with special test flag
#define TESTING
#include "pebble-darkroom.c"

// Test fixtures
static void setup(void) {
  // Initialize the app before each test
  mock_init_app();
  init();
}

static void teardown(void) {
  // Clean up after each test
  deinit();
  mock_deinit_app();
}

// Timer management tests
static void test_timer_initialization(void) {
  // Test timer initial states
  TEST_ASSERT_EQUAL(false, s_timer1.running);
  TEST_ASSERT_EQUAL(false, s_timer1.paused);
  TEST_ASSERT_EQUAL(MODE_PRINT, s_timer1.mode);
  TEST_ASSERT_EQUAL(STAGE_DEVELOP, s_timer1.stage);
  TEST_ASSERT_EQUAL(print_times[STAGE_DEVELOP], s_timer1.seconds_remaining);
  
  TEST_ASSERT_EQUAL(false, s_timer2.running);
  TEST_ASSERT_EQUAL(false, s_timer2.paused);
  TEST_ASSERT_EQUAL(MODE_PRINT, s_timer2.mode);
  TEST_ASSERT_EQUAL(STAGE_DEVELOP, s_timer2.stage);
  TEST_ASSERT_EQUAL(print_times[STAGE_DEVELOP], s_timer2.seconds_remaining);
}

static void test_reset_timer(void) {
  // Setup a running timer
  s_timer1.running = true;
  s_timer1.paused = false;
  s_timer1.stage = STAGE_FIX;
  s_timer1.seconds_remaining = 42;
  s_timer1.timer_handle = (AppTimer*)1; // Mock handle
  
  // Reset timer
  reset_timer(&s_timer1);
  
  // Verify reset state
  TEST_ASSERT_EQUAL(false, s_timer1.running);
  TEST_ASSERT_EQUAL(false, s_timer1.paused);
  TEST_ASSERT_EQUAL(STAGE_DEVELOP, s_timer1.stage);
  TEST_ASSERT_EQUAL(print_times[STAGE_DEVELOP], s_timer1.seconds_remaining);
  TEST_ASSERT_NULL(s_timer1.timer_handle);
}

static void test_pause_resume_timer(void) {
  // Setup a running timer
  s_timer1.running = true;
  s_timer1.paused = false;
  s_timer1.timer_handle = (AppTimer*)1; // Mock handle
  
  // Pause timer
  pause_timer(&s_timer1);
  
  // Verify paused state
  TEST_ASSERT_EQUAL(false, s_timer1.running);
  TEST_ASSERT_EQUAL(true, s_timer1.paused);
  TEST_ASSERT_NULL(s_timer1.timer_handle);
  
  // Resume timer
  mock_expect_app_timer_register();
  resume_timer(&s_timer1);
  
  // Verify resumed state
  TEST_ASSERT_EQUAL(true, s_timer1.running);
  TEST_ASSERT_EQUAL(false, s_timer1.paused);
  TEST_ASSERT_NOT_NULL(s_timer1.timer_handle);
}

// Timer callback tests
static void test_timer_callback_counting_down(void) {
  // Setup a running timer
  s_timer1.running = true;
  s_timer1.seconds_remaining = 10;
  
  // Call timer callback
  mock_expect_app_timer_register();
  timer_callback(&s_timer1);
  
  // Verify timer decremented and registered again
  TEST_ASSERT_EQUAL(9, s_timer1.seconds_remaining);
  TEST_ASSERT_NOT_NULL(s_timer1.timer_handle);
}

static void test_timer_callback_stage_completion(void) {
  // Setup a nearly complete timer
  s_timer1.running = true;
  s_timer1.seconds_remaining = 0;
  s_timer1.stage = STAGE_DEVELOP;
  s_settings.vibration_enabled = true;
  
  // Expect vibration
  mock_expect_vibes_pulse();
  
  // Call timer callback
  timer_callback(&s_timer1);
  
  // Verify stage advanced but timer not running
  TEST_ASSERT_EQUAL(STAGE_STOP, s_timer1.stage);
  TEST_ASSERT_EQUAL(false, s_timer1.running);
  TEST_ASSERT_EQUAL(print_times[STAGE_STOP], s_timer1.seconds_remaining);
  TEST_ASSERT_NULL(s_timer1.timer_handle);
}

static void test_timer_callback_final_stage_completion(void) {
  // Setup a timer at final stage
  s_timer1.running = true;
  s_timer1.seconds_remaining = 0;
  s_timer1.stage = STAGE_WASH;
  s_settings.vibration_enabled = true;
  
  // Expect vibration
  mock_expect_vibes_pulse();
  
  // Call timer callback
  timer_callback(&s_timer1);
  
  // Verify timer reset to start
  TEST_ASSERT_EQUAL(STAGE_DEVELOP, s_timer1.stage);
  TEST_ASSERT_EQUAL(false, s_timer1.running);
  TEST_ASSERT_EQUAL(print_times[STAGE_DEVELOP], s_timer1.seconds_remaining);
  TEST_ASSERT_NULL(s_timer1.timer_handle);
}

static void test_delayed_vibration_callback(void) {
  // Setup a completed, non-running timer
  s_timer1.running = false;
  s_settings.vibration_enabled = true;
  
  // Expect vibration
  mock_expect_vibes_pulse();
  
  // Call delayed vibration callback
  delayed_vibration_callback(&s_timer1);
  
  // Test no vibration when timer already running
  s_timer1.running = true;
  
  // Should not vibrate, so no expectation needed
  delayed_vibration_callback(&s_timer1);
}

// Button handler tests
static void test_down_click_handler_start_timer(void) {
  // Setup a reset timer
  s_active_timer = 1;
  s_timer1.running = false;
  s_timer1.paused = false;
  
  // Expect timer registration
  mock_expect_app_timer_register();
  
  // Call click handler
  down_click_handler(NULL, NULL);
  
  // Verify timer started
  TEST_ASSERT_EQUAL(true, s_timer1.running);
  TEST_ASSERT_NOT_NULL(s_timer1.timer_handle);
}

static void test_down_click_handler_pause_timer(void) {
  // Setup a running timer
  s_active_timer = 1;
  s_timer1.running = true;
  s_timer1.paused = false;
  s_timer1.timer_handle = (AppTimer*)1; // Mock handle
  
  // Call click handler
  down_click_handler(NULL, NULL);
  
  // Verify timer paused
  TEST_ASSERT_EQUAL(false, s_timer1.running);
  TEST_ASSERT_EQUAL(true, s_timer1.paused);
  TEST_ASSERT_NULL(s_timer1.timer_handle);
}

static void test_down_click_handler_resume_timer(void) {
  // Setup a paused timer
  s_active_timer = 1;
  s_timer1.running = false;
  s_timer1.paused = true;
  
  // Expect timer registration
  mock_expect_app_timer_register();
  
  // Call click handler
  down_click_handler(NULL, NULL);
  
  // Verify timer resumed
  TEST_ASSERT_EQUAL(true, s_timer1.running);
  TEST_ASSERT_EQUAL(false, s_timer1.paused);
  TEST_ASSERT_NOT_NULL(s_timer1.timer_handle);
}

static void test_down_long_click_handler(void) {
  // Setup a running timer
  s_active_timer = 1;
  s_timer1.running = true;
  s_timer1.timer_handle = (AppTimer*)1; // Mock handle
  s_timer1.stage = STAGE_FIX;
  
  // Call long click handler
  down_long_click_handler(NULL, NULL);
  
  // Verify timer reset
  TEST_ASSERT_EQUAL(false, s_timer1.running);
  TEST_ASSERT_EQUAL(false, s_timer1.paused);
  TEST_ASSERT_EQUAL(STAGE_DEVELOP, s_timer1.stage);
  TEST_ASSERT_NULL(s_timer1.timer_handle);
}

static void test_down_double_click_handler(void) {
  // Setup a timer in film mode
  s_active_timer = 1;
  s_timer1.mode = MODE_FILM;
  
  // Call double click handler
  down_double_click_handler(NULL, NULL);
  
  // Verify mode toggled
  TEST_ASSERT_EQUAL(MODE_PRINT, s_timer1.mode);
  
  // Call again to toggle back
  down_double_click_handler(NULL, NULL);
  
  // Verify mode toggled back
  TEST_ASSERT_EQUAL(MODE_FILM, s_timer1.mode);
}

static void test_up_double_click_handler(void) {
  // Setup initial active timer
  s_active_timer = 1;
  
  // Call double click handler
  up_double_click_handler(NULL, NULL);
  
  // Verify active timer switched
  TEST_ASSERT_EQUAL(2, s_active_timer);
  
  // Call again to switch back
  up_double_click_handler(NULL, NULL);
  
  // Verify active timer switched back
  TEST_ASSERT_EQUAL(1, s_active_timer);
}

// Settings tests
static void test_save_load_settings(void) {
  // Change settings from defaults
  s_settings.vibration_enabled = false;
  s_settings.backlight_enabled = true;
  s_settings.invert_timer1_colors = true;
  film_times[0] = 999;
  print_times[0] = 888;
  
  // Save settings
  save_settings();
  
  // Reset settings to defaults
  s_settings.vibration_enabled = true;
  s_settings.backlight_enabled = false;
  s_settings.invert_timer1_colors = false;
  film_times[0] = 300;
  print_times[0] = 60;
  
  // Load settings
  load_settings();
  
  // Verify settings restored
  TEST_ASSERT_EQUAL(false, s_settings.vibration_enabled);
  TEST_ASSERT_EQUAL(true, s_settings.backlight_enabled);
  TEST_ASSERT_EQUAL(true, s_settings.invert_timer1_colors);
  TEST_ASSERT_EQUAL(999, film_times[0]);
  TEST_ASSERT_EQUAL(888, print_times[0]);
}

// Run all tests
int test_main(void) {
  UNITY_BEGIN();
  
  // Timer state tests
  RUN_TEST(test_timer_initialization);
  RUN_TEST(test_reset_timer);
  RUN_TEST(test_pause_resume_timer);
  
  // Timer callback tests
  RUN_TEST(test_timer_callback_counting_down);
  RUN_TEST(test_timer_callback_stage_completion);
  RUN_TEST(test_timer_callback_final_stage_completion);
  RUN_TEST(test_delayed_vibration_callback);
  
  // Button handler tests
  RUN_TEST(test_down_click_handler_start_timer);
  RUN_TEST(test_down_click_handler_pause_timer);
  RUN_TEST(test_down_click_handler_resume_timer);
  RUN_TEST(test_down_long_click_handler);
  RUN_TEST(test_down_double_click_handler);
  RUN_TEST(test_up_double_click_handler);
  
  // Settings tests
  RUN_TEST(test_save_load_settings);
  
  return UNITY_END();
}
