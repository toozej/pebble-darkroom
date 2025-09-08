/** Simple Unity C Test Framework
 * 
 * This is a minimal implementation of the Unity unit test framework for C.
 * Designed for embedded systems and simple test environments.
 * 
 * To use:
 * 1. Include this header in your test files
 * 2. Define test functions with TEST_ASSERT macros
 * 3. Call your test functions from main()
 */

#ifndef UNITY_H_INCLUDED
#define UNITY_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

// Type definitions
typedef int UNITY_INT;
typedef unsigned int UNITY_UINT;
typedef float UNITY_FLOAT;
typedef double UNITY_DOUBLE;
typedef void* UNITY_PTR_TYPE;

// Configuration options
#define UNITY_OUTPUT_CHAR(c) putchar(c)
#define UNITY_OUTPUT_FLUSH() fflush(stdout)

// Test state
typedef struct {
    int numTests;
    int numFails;
    int numIgnores;
    char* currentTestName;
    char* currentFile;
    int currentLine;
    jmp_buf abortFrame;
    bool testFailed;
} Unity;

extern Unity UnityGlobal;

// External functions - to be implemented by test runner
extern void setUp(void);
extern void tearDown(void);

// Basic assertion macros
#define TEST_ASSERT_EQUAL_INT(expected, actual) UnityAssertEqualInt((expected), (actual), __LINE__, #actual)
#define TEST_ASSERT_EQUAL_UINT(expected, actual) UnityAssertEqualInt((int)(expected), (int)(actual), __LINE__, #actual)
#define TEST_ASSERT_EQUAL_STRING(expected, actual) UnityAssertEqualString((expected), (actual), __LINE__)
#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) UnityAssertEqualMemory((expected), (actual), (len), __LINE__)
#define TEST_ASSERT_TRUE(condition) UnityAssert((condition), __LINE__, #condition)
#define TEST_ASSERT_FALSE(condition) UnityAssert(!(condition), __LINE__, #condition)
#define TEST_ASSERT_NULL(pointer) UnityAssertPointerNull((pointer), __LINE__, #pointer)
#define TEST_ASSERT_NOT_NULL(pointer) UnityAssertPointerNotNull((pointer), __LINE__, #pointer)
#define TEST_ASSERT_EQUAL_INT_ARRAY(expected, actual, num_elements) UnityAssertEqualIntArray((expected), (actual), (num_elements), __LINE__)
#define TEST_FAIL() UnityFail(__LINE__, "Test failed")
#define TEST_IGNORE() UnityIgnore(__LINE__, "Test ignored")

// Test function macros
#define TEST(name) void name(void)
#define RUN_TEST(testFunc) { \
    setUp(); \
    UnityGlobal.currentTestName = #testFunc; \
    UnityGlobal.testFailed = false; \
    if (setjmp(UnityGlobal.abortFrame) == 0) { \
        testFunc(); \
        UnityGlobal.numTests++; \
        if (!UnityGlobal.testFailed) { \
            UNITY_OUTPUT_CHAR('.'); \
        } \
    } else { \
        UnityGlobal.numTests++; \
        UnityGlobal.numFails++; \
        UNITY_OUTPUT_CHAR('F'); \
    } \
    tearDown(); \
}

// Unity function declarations
void UnityBegin(const char* testFile);
int UnityEnd(void);
void UnityAssert(bool condition, int line, const char* message);
void UnityAssertEqualInt(int expected, int actual, int line, const char* actualStr);
void UnityAssertEqualString(const char* expected, const char* actual, int line);
void UnityAssertEqualMemory(const void* expected, const void* actual, size_t len, int line);
void UnityAssertPointerNull(const void* ptr, int line, const char* ptrStr);
void UnityAssertPointerNotNull(const void* ptr, int line, const char* ptrStr);
void UnityAssertEqualIntArray(const int* expected, const int* actual, int num_elements, int line);
void UnityFail(int line, const char* message);
void UnityIgnore(int line, const char* message);

// Mock types for Pebble API (simplified for unit testing)
typedef struct { int dummy; } Window;
typedef struct { int dummy; } Layer;
typedef struct { int dummy; } TextLayer;
typedef struct { int dummy; } GContext;
typedef struct { int x, y, w, h; } GRect;
typedef struct { uint8_t r, g, b; } GColor;
typedef uint32_t AppTimer;
typedef void (*AppTimerCallback)(void*);
typedef struct { int dummy; } GBitmap;
typedef struct { int dummy; } MenuLayer;
typedef struct { int dummy; } NumberWindow;
typedef struct { int dummy; } NumberWindowCallbacks;
typedef struct { int dummy; } ClickRecognizerRef;

// Mock Pebble function declarations
Window* window_create(void);
void window_destroy(Window* window);
Layer* window_get_root_layer(Window* window);
void window_set_background_color(Window* window, GColor color);
void window_set_click_config_provider(Window* window, void* config);
void text_layer_set_text(TextLayer* layer, const char* text);
void text_layer_set_font(TextLayer* layer, GBitmap* font);
void text_layer_set_text_alignment(TextLayer* layer, int alignment);
void text_layer_set_background_color(TextLayer* layer, GColor color);
void text_layer_set_text_color(TextLayer* layer, GColor color);
TextLayer* text_layer_create(GRect location);
void text_layer_destroy(TextLayer* layer);
Layer* text_layer_get_layer(TextLayer* layer);
Layer* layer_create(GRect bounds);
void layer_destroy(Layer* layer);
void layer_add_child(Layer* parent, Layer* child);
void layer_set_update_proc(Layer* layer, void* updateProc);
void layer_mark_dirty(Layer* layer);
GRect layer_get_bounds(Layer* layer);
void graphics_context_set_fill_color(GContext* ctx, GColor color);
void graphics_context_set_stroke_color(GContext* ctx, GColor color);
void graphics_context_set_text_color(GContext* ctx, GColor color);
void graphics_fill_rect(GContext* ctx, GRect rect, int radius, int corners);
void graphics_draw_rect(GContext* ctx, GRect rect);
void menu_cell_basic_draw(GContext* ctx, Layer* cell_layer, const char* title, void* subtitle, void* icon);
void menu_cell_basic_header_draw(GContext* ctx, Layer* cell_layer, const char* title);
AppTimer* app_timer_register(uint32_t milliseconds, AppTimerCallback callback, void* data);
void app_timer_cancel(AppTimer* timer);
void vibes_short_pulse(void);
void vibes_double_pulse(void);
void light_enable(bool enable);
GBitmap* fonts_get_system_font(const char* key);
GRect GRect_zero(void);

// Mock other constants
#define GCornerNone 0
#define FONT_KEY_GOTHIC_18_BOLD 0
#define FONT_KEY_GOTHIC_14 0
#define FONT_KEY_BITHAM_42_BOLD 0
#define GTextAlignmentCenter 0
#define GColorClear 0
#define GColorWhite 0
#define GColorBlack 0
#define MENU_CELL_BASIC_HEADER_HEIGHT 16

// Mock Pebble API types and constants
#define BUTTON_ID_SELECT 0
#define BUTTON_ID_UP 1
#define BUTTON_ID_DOWN 2

#endif // UNITY_H_INCLUDED