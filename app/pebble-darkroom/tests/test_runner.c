#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// Test function declarations
void test_settings(void);
void test_timer(void);
void test_display(void);

// Test suite setup and teardown
void suiteSetup(void) {
    printf("\n=== Pebble Darkroom Unit Tests ===\n");
}

void suiteTearDown(void) {
    printf("\n=== Test Suite Complete ===\n");
}

// Main test runner function
int main(void) {
    UnityBegin(__FILE__);
    
    // Run all test groups with proper setjmp wrapping
    if (setjmp(UnityGlobal.abortFrame) == 0) {
        test_settings();
    } else {
        UnityGlobal.numTests++;
        UnityGlobal.numFails++;
        printf("F\n");
    }
    
    if (setjmp(UnityGlobal.abortFrame) == 0) {
        test_timer();
    } else {
        UnityGlobal.numTests++;
        UnityGlobal.numFails++;
        printf("F\n");
    }
    
    if (setjmp(UnityGlobal.abortFrame) == 0) {
        test_display();
    } else {
        UnityGlobal.numTests++;
        UnityGlobal.numFails++;
        printf("F\n");
    }
    
    return UnityEnd();
}

// Simple printf replacement for better compatibility
int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}