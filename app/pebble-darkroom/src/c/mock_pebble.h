// mock_pebble.h - Mocking framework for Pebble SDK functions
#ifndef MOCK_PEBBLE_H
#define MOCK_PEBBLE_H

#include <pebble.h>

/**
 * Initialize the mock framework
 */
void mock_init_app(void);

/**
 * Clean up the mock framework
 */
void mock_deinit_app(void);

/**
 * Set expectation for app_timer_register call
 * Returns a mock timer handle
 */
void mock_expect_app_timer_register(void);

/**
 * Set expectation for a vibes call (short or double pulse)
 */
void mock_expect_vibes_pulse(void);

/**
 * Mock implementation of Pebble SDK functions for testing
 */
#ifdef TESTING
  // Replace actual SDK functions with test implementations
  #define app_timer_register mock_app_timer_register
  #define app_timer_cancel mock_app_timer_cancel
  #define vibes_short_pulse mock_vibes_short_pulse
  #define vibes_double_pulse mock_vibes_double_pulse
  #define persist_exists mock_persist_exists
  #define persist_read_data mock_persist_read_data
  #define persist_write_data mock_persist_write_data
  
  // Mock implementations
  AppTimer* mock_app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void* callback_data);
  bool mock_app_timer_cancel(AppTimer *timer_handle);
  void mock_vibes_short_pulse(void);
  void mock_vibes_double_pulse(void);
  bool mock_persist_exists(const uint32_t key);
  int mock_persist_read_data(const uint32_t key, void *buffer, const size_t buffer_size);
  int mock_persist_write_data(const uint32_t key, const void *data, const size_t size);
#endif

#endif // MOCK_PEBBLE_H
