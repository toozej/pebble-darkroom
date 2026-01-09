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
    .paper_type = PAPER_RC,
    .max_stages = 4,
    .seconds_remaining = 0,
    .timer_handle = NULL
};

static TimerState s_timer2 = {
    .running = false,
    .paused = false,
    .mode = MODE_PRINT,
    .stage = STAGE_DEVELOP,
    .paper_type = PAPER_FIBER,
    .max_stages = 6,
    .seconds_remaining = 0,
    .timer_handle = NULL
};

// Timer times arrays
static int film_times[4] = {300, 60, 300, 300};  // Develop, Stop, Fix, Wash

// RC paper timing (4 stages)
static int rc_print_times[4] = {60, 30, 300, 300};  // Develop, Stop, Fix, Wash

// Fiber paper timing (6 stages)
static int fiber_print_times[6] = {120, 30, 120, 300, 120, 900};  // Develop, Stop, Fix, Wash, HC, Wash2


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

int* get_rc_print_times(void) {
    return rc_print_times;
}

int* get_fiber_print_times(void) {
    return fiber_print_times;
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
    printf("Saving RC print times: [%d, %d, %d, %d]\n",
           rc_print_times[0], rc_print_times[1], rc_print_times[2], rc_print_times[3]);
    printf("Saving Fiber print times: [%d, %d, %d, %d, %d, %d]\n",
           fiber_print_times[0], fiber_print_times[1], fiber_print_times[2], 
           fiber_print_times[3], fiber_print_times[4], fiber_print_times[5]);
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

// Timer configuration structure
typedef struct {
    TimerMode mode;
    PaperType paper_type;
    int *timing_array;
    int stage_count;
    const char *paper_name;
} TimerConfig;

// Timer configuration lookup helper function
static TimerConfig get_timer_config(int timer_number, TimerMode mode) {
    if (mode == MODE_FILM) {
        return (TimerConfig){
            .mode = MODE_FILM,
            .paper_type = PAPER_RC, // Not applicable for film
            .timing_array = film_times,
            .stage_count = 4,
            .paper_name = "Film"
        };
    } else {
        // Timer 1 = RC, Timer 2 = Fiber
        if (timer_number == 1) {
            return (TimerConfig){
                .mode = MODE_PRINT,
                .paper_type = PAPER_RC,
                .timing_array = rc_print_times,
                .stage_count = 4,
                .paper_name = "RC"
            };
        } else {
            return (TimerConfig){
                .mode = MODE_PRINT,
                .paper_type = PAPER_FIBER,
                .timing_array = fiber_print_times,
                .stage_count = 6,
                .paper_name = "FB"
            };
        }
    }
}

// Helper function to get max stage for current timer
TimerStage get_max_stage(TimerState *timer) {
    if (timer->mode == MODE_FILM) {
        return STAGE_WASH;  // Film has 4 stages ending at WASH
    } else if (timer->paper_type == PAPER_RC) {
        return STAGE_WASH;  // RC paper has 4 stages ending at WASH
    } else {
        return STAGE_WASH2;  // Fiber paper has 6 stages ending at WASH2
    }
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
    
    // Determine which timer this is (1 or 2)
    int timer_number = (timer == &s_timer1) ? 1 : 2;
    
    // Get timer configuration
    TimerConfig config = get_timer_config(timer_number, timer->mode);
    
    // Update timer properties based on configuration
    timer->paper_type = config.paper_type;
    timer->max_stages = config.stage_count;
    
    // Set initial timing for develop stage
    timer->seconds_remaining = config.timing_array[STAGE_DEVELOP];
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
        case STAGE_HYPO_CLEAR: stage_str = "HYPO_CLEAR"; break;
        case STAGE_WASH2: stage_str = "WASH2"; break;
        default: stage_str = "UNKNOWN"; break;
    }
    snprintf(buffer, sizeof(buffer), 
             "Timer{running=%s,paused=%s,mode=%s,stage=%s,seconds=%d}",
             timer->running ? "true" : "false",
             timer->paused ? "true" : "false",
             mode_str, stage_str, timer->seconds_remaining);
    return buffer;
}
