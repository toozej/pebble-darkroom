#include "settings.h"
#include <stdio.h>
#include <stdbool.h>

// Global settings and timer states (for testing purposes)
static Settings s_settings = {
    .vibration_enabled = true,
    .backlight_enabled = false,
    .invert_timer1_colors = false,
    .invert_timer2_colors = false,
    .invert_menu_colors = false
};

static TimerState s_timer1 = {
    .running = false,
    .paused = false,
    .mode = MODE_PRINT,
    .stage = STAGE_DEVELOP,
    .seconds_remaining = 0,
    .timer_handle = NULL
};

static TimerState s_timer2 = {
    .running = false,
    .paused = false,
    .mode = MODE_PRINT,
    .stage = STAGE_DEVELOP,
    .seconds_remaining = 0,
    .timer_handle = NULL
};

// Timer times arrays
static int film_times[4] = {300, 60, 300, 300};  // Develop, Stop, Fix, Wash
static int print_times[4] = {60, 30, 300, 300};  // Develop, Stop, Fix, Wash


// Getter functions for testing
Settings* get_settings(void) {
    return &s_settings;
}

TimerState* get_timer1(void) {
    return &s_timer1;
}

TimerState* get_timer2(void) {
    return &s_timer2;
}

int* get_film_times(void) {
    return film_times;
}

int* get_print_times(void) {
    return print_times;
}

// Persistent storage functions
void save_settings(void) {
    // Simulate persist_write_data for testing
    // In real app, this would use actual Pebble persistence
    extern bool persist_write_called;
    extern int persist_key;
    persist_write_called = true;
    persist_key = SETTINGS_KEY;
    printf("Saving settings: vibration=%s, backlight=%s\n",
           s_settings.vibration_enabled ? "true" : "false",
           s_settings.backlight_enabled ? "true" : "false");
}

void load_settings(void) {
    // Simulate persist_read_data for testing
    // In real app, this would load from actual Pebble persistence
    // For tests, use default values
    extern bool persist_read_called;
    extern int persist_key;
    persist_read_called = true;
    persist_key = SETTINGS_KEY;
    printf("Loading default settings\n");
}

// Timer control functions
void reset_timer(TimerState *timer) {
    if (timer->timer_handle) {
        // Simulate timer cancellation
        timer->timer_handle = NULL;
    }
    timer->running = false;
    timer->paused = false;
    timer->stage = STAGE_DEVELOP;
    timer->seconds_remaining = (timer->mode == MODE_FILM) ? 
        film_times[STAGE_DEVELOP] : print_times[STAGE_DEVELOP];
}

void pause_timer(TimerState *timer) {
    if (timer->timer_handle) {
        extern bool app_timer_cancel_called;
        app_timer_cancel_called = true;
        timer->timer_handle = NULL;
    }
    timer->paused = true;
    timer->running = false;
}

void resume_timer(TimerState *timer) {
    timer->paused = false;
    timer->running = true;
    // Simulate timer registration
    extern bool app_timer_register_called;
    app_timer_register_called = true;
    timer->timer_handle = (void*)0xDEADBEEF;  // Mock handle
}

// Utility functions for test verification
char* settings_to_string(Settings *settings) {
    static char buffer[128];
    snprintf(buffer, sizeof(buffer), 
             "Settings{vib=%s,backlight=%s,t1_inv=%s,t2_inv=%s,menu_inv=%s}",
             settings->vibration_enabled ? "true" : "false",
             settings->backlight_enabled ? "true" : "false",
             settings->invert_timer1_colors ? "true" : "false",
             settings->invert_timer2_colors ? "true" : "false",
             settings->invert_menu_colors ? "true" : "false");
    return buffer;
}

char* timer_to_string(TimerState *timer) {
    static char buffer[128];
    const char* mode_str = (timer->mode == MODE_FILM) ? "FILM" : "PRINT";
    const char* stage_str;
    switch (timer->stage) {
        case STAGE_DEVELOP: stage_str = "DEVELOP"; break;
        case STAGE_STOP: stage_str = "STOP"; break;
        case STAGE_FIX: stage_str = "FIX"; break;
        case STAGE_WASH: stage_str = "WASH"; break;
        default: stage_str = "UNKNOWN"; break;
    }
    snprintf(buffer, sizeof(buffer), 
             "Timer{running=%s,paused=%s,mode=%s,stage=%s,seconds=%d}",
             timer->running ? "true" : "false",
             timer->paused ? "true" : "false",
             mode_str, stage_str, timer->seconds_remaining);
    return buffer;
}