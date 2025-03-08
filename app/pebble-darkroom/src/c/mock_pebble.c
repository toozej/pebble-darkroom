// mock_pebble.c - Implementation of Pebble mocking framework
#include "mock_pebble.h"
#include "unity/unity.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pebble.h>

// Mock storage for persistent data
static uint8_t mock_storage[4][256];
static bool mock_exists[4] = {false};

// Mock expectations
static bool expect_app_timer = false;
static bool expect_vibes = false;

// Mock app timer - Change to a pointer instead of a static instance
static AppTimer* mock_timer_ptr;

void mock_init_app(void) {
  // Reset expectations
  expect_app_timer = false;
  expect_vibes = false;
  
  // Reset mock storage
  memset(mock_storage, 0, sizeof(mock_storage));
  for (int i = 0; i < 4; i++) {
    mock_exists[i] = false;
  }
  
  // Initialize mock timer pointer
  mock_timer_ptr = malloc(sizeof(void*));
}

void mock_deinit_app(void) {
  // Verify all expectations were met
  TEST_ASSERT_FALSE_MESSAGE(expect_app_timer, "Expected app_timer_register was not called");
  TEST_ASSERT_FALSE_MESSAGE(expect_vibes, "Expected vibes function was not called");
  
  // Free mock timer pointer
  if (mock_timer_ptr) {
    free(mock_timer_ptr);
    mock_timer_ptr = NULL;
  }
}

void mock_expect_app_timer_register(void) {
  expect_app_timer = true;
}

void mock_expect_vibes_pulse(void) {
  expect_vibes = true;
}

// Mock implementations of Pebble SDK functions

AppTimer* mock_app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void* callback_data) {
  TEST_ASSERT_TRUE_MESSAGE(expect_app_timer, "Unexpected call to app_timer_register");
  expect_app_timer = false;
  return mock_timer_ptr;
}

bool mock_app_timer_cancel(AppTimer *timer_handle) {
  return true;
}

void mock_vibes_short_pulse(void) {
  TEST_ASSERT_TRUE_MESSAGE(expect_vibes, "Unexpected call to vibes_short_pulse");
  expect_vibes = false;
}

void mock_vibes_double_pulse(void) {
  TEST_ASSERT_TRUE_MESSAGE(expect_vibes, "Unexpected call to vibes_double_pulse");
  expect_vibes = false;
}

bool mock_persist_exists(const uint32_t key) {
  TEST_ASSERT_LESS_THAN(4, key);
  return mock_exists[key];
}

int mock_persist_read_data(const uint32_t key, void *buffer, const size_t buffer_size) {
  TEST_ASSERT_LESS_THAN(4, key);
  if (!mock_exists[key]) {
    return E_DOES_NOT_EXIST;
  }
  memcpy(buffer, mock_storage[key], buffer_size);
  return buffer_size;
}

int mock_persist_write_data(const uint32_t key, const void *data, const size_t size) {
  TEST_ASSERT_LESS_THAN(4, key);
  TEST_ASSERT_LESS_OR_EQUAL(256, size);
  memcpy(mock_storage[key], data, size);
  mock_exists[key] = true;
  return size;
}
