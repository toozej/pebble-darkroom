
/* Unity implementation - to be compiled once */
#define UNITY_IMPLEMENTATION
#include "unity.h"

// Mock implementation for app_timer_register
AppTimer* app_timer_register(uint32_t milliseconds, AppTimerCallback callback, void* data) {
    // Simple mock - return a dummy non-NULL timer handle
    static AppTimer* dummy_handle = (AppTimer*)0xDEADBEEF;
    return dummy_handle;
}

Unity UnityGlobal = {0};

// Unity function implementations
void UnityBegin(const char* testFile) {
    UnityGlobal.numTests = 0;
    UnityGlobal.numFails = 0;
    UnityGlobal.numIgnores = 0;
    UnityGlobal.currentFile = (char*)testFile;
    
    // Initialize abort frame for longjmp
    if (setjmp(UnityGlobal.abortFrame) != 0) {
        // This should not happen in UnityBegin
        printf("Error: Unexpected longjmp in UnityBegin\n");
        exit(1);
    }
    
    printf("\nRunning tests from %s\n", testFile);
}

int UnityEnd(void) {
    printf("\n\nTest Results:\n");
    printf("Tests run: %d\n", UnityGlobal.numTests);
    if (UnityGlobal.numFails > 0) {
        printf("Failures: %d\n", UnityGlobal.numFails);
        return -1;
    } else {
        printf("All tests passed!\n");
        return 0;
    }
}

void UnityAssert(bool condition, int line, const char* message) {
    if (!condition) {
        UnityGlobal.testFailed = true;
        printf("\nFAIL: %s at line %d: %s\n", UnityGlobal.currentTestName, line, message);
        longjmp(UnityGlobal.abortFrame, 1);
    }
}

void UnityAssertEqualInt(int expected, int actual, int line, const char* actualStr) {
    if (expected != actual) {
        UnityGlobal.testFailed = true;
        printf("\nFAIL: %s at line %d: expected %d but was %d (%s)\n", 
               UnityGlobal.currentTestName, line, expected, actual, actualStr);
        longjmp(UnityGlobal.abortFrame, 1);
    }
}

void UnityAssertEqualString(const char* expected, const char* actual, int line) {
    if (expected == NULL && actual == NULL) return;
    if (expected == NULL || actual == NULL || strcmp(expected, actual) != 0) {
        UnityGlobal.testFailed = true;
        printf("\nFAIL: %s at line %d: expected '%s' but was '%s'\n", 
               UnityGlobal.currentTestName, line, expected ? expected : "NULL", 
               actual ? actual : "NULL");
        longjmp(UnityGlobal.abortFrame, 1);
    }
}

void UnityAssertEqualMemory(const void* expected, const void* actual, size_t len, int line) {
    if (memcmp(expected, actual, len) != 0) {
        UnityGlobal.testFailed = true;
        printf("\nFAIL: %s at line %d: memory comparison failed\n", 
               UnityGlobal.currentTestName, line);
        longjmp(UnityGlobal.abortFrame, 1);
    }
}

void UnityAssertPointerNull(const void* ptr, int line, const char* ptrStr) {
    if (ptr != NULL) {
        UnityGlobal.testFailed = true;
        printf("\nFAIL: %s at line %d: expected NULL but was %p (%s)\n", 
               UnityGlobal.currentTestName, line, ptr, ptrStr);
        longjmp(UnityGlobal.abortFrame, 1);
    }
}

void UnityAssertPointerNotNull(const void* ptr, int line, const char* ptrStr) {
    if (ptr == NULL) {
        UnityGlobal.testFailed = true;
        printf("\nFAIL: %s at line %d: expected non-NULL but was NULL (%s)\n",
               UnityGlobal.currentTestName, line, ptrStr);
        longjmp(UnityGlobal.abortFrame, 1);
    }
}