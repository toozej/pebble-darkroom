#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

// Settings structure
typedef struct {
    bool vibration_enabled;
    bool backlight_enabled;
    bool invert_timer1_colors;
    bool invert_timer2_colors;
    bool invert_menu_colors;
} Settings;

// Timer modes and stages
typedef enum {
    MODE_FILM,
    MODE_PRINT
} TimerMode;

typedef enum {
    STAGE_DEVELOP,
    STAGE_STOP,
    STAGE_FIX,
    STAGE_WASH,
    STAGE_WASH1,
    STAGE_HYPO_CLEAR,
    STAGE_WASH2
} TimerStage;

typedef enum {
    PAPER_RC,
    PAPER_FIBER
} PaperType;

// Timer state structure
typedef struct {
    bool running;
    bool paused;
    TimerMode mode;
    TimerStage stage;
    PaperType paper_type;
    int max_stages;
    int seconds_remaining;
    void *timer_handle;
} TimerState;

// Persistent storage keys
#define SETTINGS_KEY 1
#define FILM_TIMES_KEY 2
#define PRINT_TIMES_KEY 3
#define RC_PRINT_TIMES_KEY 4
#define FIBER_PRINT_TIMES_KEY 5

// Function declarations for testing
Settings* get_settings(void);
TimerState* get_timer1(void);
TimerState* get_timer2(void);
int* get_film_times(void);
int* get_print_times(void);
int* get_rc_print_times(void);
int* get_fiber_print_times(void);
void save_settings(void);
void load_settings(void);
void reset_timer(TimerState *timer);
void pause_timer(TimerState *timer);
void resume_timer(TimerState *timer);

// Utility functions for testing
char* timer_to_string(TimerState *timer);
char* settings_to_string(Settings *settings);

#endif // SETTINGS_H