#include <pebble.h>

#define SETTINGS_KEY 1
#define FILM_TIMES_KEY 2
#define RC_PRINT_TIMES_KEY 4
#define FIBER_PRINT_TIMES_KEY 5

// Window and layer handles
static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_timer_layer;
static TextLayer *s_mode_layer;
static TextLayer *s_timer_name_layer;

// Menu windows and layers
static Window *s_menu_window;
static MenuLayer *s_menu_layer;

// Timer states
typedef enum {
    MODE_FILM,
    MODE_PRINT
} TimerMode;

typedef enum {
    STAGE_DEVELOP,
    STAGE_STOP,
    STAGE_FIX,
    STAGE_WASH,
    STAGE_HYPO_CLEAR,
    STAGE_WASH2
} TimerStage;

typedef enum {
    PAPER_RC,
    PAPER_FIBER
} PaperType;

typedef struct {
    bool running;
    bool paused;
    TimerMode mode;
    TimerStage stage;
    PaperType paper_type;
    int max_stages;
    int seconds_remaining;
    AppTimer *timer_handle;
} TimerState;

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

// Currently displayed timer (1 or 2)
static int s_active_timer = 1;

// Settings
typedef struct {
    bool vibration_enabled;
    bool backlight_enabled;
    bool invert_timer1_colors;
    bool invert_timer2_colors;
    bool invert_menu_colors;
} Settings;

static Settings s_settings = {
    .vibration_enabled = true,
    .backlight_enabled = false,
    .invert_timer1_colors = false,  // Timer 1 defaults to light mode (white bg, black text)
    .invert_timer2_colors = false,  // Timer 2 defaults to dark mode (black bg, white text)
    .invert_menu_colors = false
};

// Timer settings (in seconds)
static int film_times[4] = {
    300,  // Develop: 5 mins
    60,   // Stop: 1 min
    300,  // Fix: 5 mins
    300   // Wash: 5 mins
};

// RC paper timing (4 stages)
static int rc_print_times[4] = {
    60,   // Develop: 1 min
    30,   // Stop: 30 secs
    300,  // Fix: 5 mins
    300   // Wash: 5 mins
};

// Fiber paper timing (6 stages)
static int fiber_print_times[6] = {
    120,  // Develop: 2 min
    30,   // Stop: 30 sec
    120,  // Fix: 2 min
    300,  // Wash: 5 min
    120,  // Hypo Clear: 2 min
    900   // Wash2: 15 min
};

// Persistent storage functions
static void save_settings() {
    persist_write_data(SETTINGS_KEY, &s_settings, sizeof(Settings));
    persist_write_data(FILM_TIMES_KEY, &film_times, sizeof(film_times));
    persist_write_data(RC_PRINT_TIMES_KEY, &rc_print_times, sizeof(rc_print_times));
    persist_write_data(FIBER_PRINT_TIMES_KEY, &fiber_print_times, sizeof(fiber_print_times));
}

static void load_settings() {
    if (persist_exists(SETTINGS_KEY)) {
        persist_read_data(SETTINGS_KEY, &s_settings, sizeof(Settings));
    }
    if (persist_exists(FILM_TIMES_KEY)) {
        persist_read_data(FILM_TIMES_KEY, &film_times, sizeof(film_times));
    }
    
    // Load RC and Fiber timing arrays
    if (persist_exists(RC_PRINT_TIMES_KEY)) {
        persist_read_data(RC_PRINT_TIMES_KEY, &rc_print_times, sizeof(rc_print_times));
    }
    
    if (persist_exists(FIBER_PRINT_TIMES_KEY)) {
        persist_read_data(FIBER_PRINT_TIMES_KEY, &fiber_print_times, sizeof(fiber_print_times));
    }
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

static TimerState* get_active_timer() {
    return s_active_timer == 1 ? &s_timer1 : &s_timer2;
}

static void update_timer_text() {
    TimerState *timer = get_active_timer();
    static char s_buffer[8];
    int minutes = timer->seconds_remaining / 60;
    int seconds = timer->seconds_remaining % 60;
    snprintf(s_buffer, sizeof(s_buffer), "%02d:%02d", minutes, seconds);
    text_layer_set_text(s_timer_layer, s_buffer);
}

static void update_mode_text() {
    TimerState *timer = get_active_timer();
    static char s_buffer[32];
    char mode_char = (timer->mode == MODE_FILM) ? 'F' : 'P';
    
    // Get paper type string
    const char *paper_type = "Film";
    if (timer->mode == MODE_PRINT) {
        paper_type = (timer->paper_type == PAPER_RC) ? "RC" : "FB";
    }
    
    // Get stage text
    const char *stage_text = "Unknown";
    switch (timer->stage) {
        case STAGE_DEVELOP: stage_text = "Dev"; break;
        case STAGE_STOP: stage_text = "Stop"; break;
        case STAGE_FIX: stage_text = "Fix"; break;
        case STAGE_WASH: stage_text = "Wash"; break;
        case STAGE_HYPO_CLEAR: stage_text = "HC"; break;
        case STAGE_WASH2: stage_text = "Wash2"; break;
    }
    
    // Format: [P,F] | [RC,FB] | [Stage] | [Status]
    const char *status = timer->paused ? "PAUSED" : (timer->running ? "RUNNING" : "");
    if (strlen(status) > 0) {
        snprintf(s_buffer, sizeof(s_buffer), "%c | %s | %s | %s", 
                 mode_char, paper_type, stage_text, status);
    } else {
        snprintf(s_buffer, sizeof(s_buffer), "%c | %s | %s", 
                 mode_char, paper_type, stage_text);
    }
    
    text_layer_set_text(s_mode_layer, s_buffer);
}

static void update_timer_name_text() {
    static char s_buffer[10];
    snprintf(s_buffer, sizeof(s_buffer), "Timer %d", s_active_timer);
    text_layer_set_text(s_timer_name_layer, s_buffer);
}

// Callback for delayed vibration reminder
static void delayed_vibration_callback(void *data) {
    TimerState *timer = (TimerState *)data;
    
    // Only vibrate if the timer is still not running (user hasn't started it yet)
    if (!timer->running && s_settings.vibration_enabled) {
        if (timer == &s_timer1) {
            // Timer 1: single pulse when waiting
            vibes_short_pulse();
        } else {
            // Timer 2: double pulse when waiting
            vibes_double_pulse();
        }
    }
}

// Timer callback
static void timer_callback(void *data) {
    TimerState *timer = (TimerState *)data;
    timer->timer_handle = NULL;
    
    if (timer->seconds_remaining > 0) {
        timer->seconds_remaining--;
        timer->timer_handle = app_timer_register(1000, timer_callback, timer);
    } else {
        // Vibrate when a stage completes - pattern depends on which timer
        if (s_settings.vibration_enabled) {
            if (timer == &s_timer1) {
                // Timer 1: single pulse when finished
                vibes_short_pulse();
            } else {
                // Timer 2: double pulse when finished
                vibes_double_pulse();
            }
        }
        
        // Move to next stage but don't start it automatically
        timer->stage++;
        
        // Determine which timer this is (1 or 2)
        int timer_number = (timer == &s_timer1) ? 1 : 2;
        
        // Get timer configuration to determine max stages and timing
        TimerConfig config = get_timer_config(timer_number, timer->mode);

        TimerStage max_stage;
        
        if (timer->mode == MODE_FILM) {
            max_stage = STAGE_WASH; // Film always has 4 stages ending at STAGE_WASH
        } else if (timer->paper_type == PAPER_RC) {
            max_stage = STAGE_WASH; // RC paper has 4 stages ending at STAGE_WASH
        } else {
            max_stage = STAGE_WASH2; // Fiber paper has 6 stages ending at STAGE_WASH2
        }
        
        if (timer->stage > max_stage) {
            timer->stage = STAGE_DEVELOP;
            timer->running = false;
        } else {
            // Set the time for the next stage using the configuration
            timer->seconds_remaining = config.timing_array[timer->stage];
            timer->running = false;  // Don't start running automatically
            
            // Add a delayed reminder vibration
            if (s_settings.vibration_enabled) {
                app_timer_register(2000, delayed_vibration_callback, timer);
            }
        }
    }
    
    if ((timer == &s_timer1 && s_active_timer == 1) ||
        (timer == &s_timer2 && s_active_timer == 2)) {
        update_timer_text();
        update_mode_text();
    }
    
    layer_mark_dirty(s_canvas_layer);
}

static void force_screen_refresh() {
    // Force a complete layer refresh to address screen tearing
    layer_mark_dirty(window_get_root_layer(s_main_window));
    
    // Optionally provide haptic feedback to confirm the action
    if (s_settings.vibration_enabled) {
        vibes_short_pulse();
    }
}

// Menu callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 5;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0: return 2;  // Basic Settings
        case 1: return 3;  // Color Settings
        case 2: return 4;  // Film times
        case 3: return 4;  // RC Print times
        case 4: return 5;  // Fiber Print times
        default: return 0;
    }
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0:
            menu_cell_basic_header_draw(ctx, cell_layer, "Basic Settings");
            break;
        case 1:
            menu_cell_basic_header_draw(ctx, cell_layer, "Display Settings");
            break;
        case 2:
            menu_cell_basic_header_draw(ctx, cell_layer, "Film Times");
            break;
        case 3:
            menu_cell_basic_header_draw(ctx, cell_layer, "RC Print Times");
            break;
        case 4:
            menu_cell_basic_header_draw(ctx, cell_layer, "Fiber Print Times");
            break;
    }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    char buffer[32];
    
    switch (cell_index->section) {
        case 0:
            switch (cell_index->row) {
                case 0:
                    snprintf(buffer, sizeof(buffer), "Vibration: %s", 
                             s_settings.vibration_enabled ? "On" : "Off");
                    break;
                case 1:
                    snprintf(buffer, sizeof(buffer), "Backlight: %s", 
                             s_settings.backlight_enabled ? "On" : "Off");
                    break;
            }
            break;
        case 1:
            switch (cell_index->row) {
                case 0:
                    snprintf(buffer, sizeof(buffer), "Invert Timer 1: %s",
                             s_settings.invert_timer1_colors ? "On" : "Off");
                    break;
                case 1:
                    snprintf(buffer, sizeof(buffer), "Invert Timer 2: %s",
                             s_settings.invert_timer2_colors ? "On" : "Off");
                    break;
                case 2:
                    snprintf(buffer, sizeof(buffer), "Invert Menu: %s",
                             s_settings.invert_menu_colors ? "On" : "Off");
                    break;
            }
            break;
        case 2:
            switch (cell_index->row) {
                case 0:
                    snprintf(buffer, sizeof(buffer), "Develop: %d:%02d", 
                             film_times[0]/60, film_times[0]%60);
                    break;
                case 1:
                    snprintf(buffer, sizeof(buffer), "Stop: %d:%02d", 
                             film_times[1]/60, film_times[1]%60);
                    break;
                case 2:
                    snprintf(buffer, sizeof(buffer), "Fix: %d:%02d", 
                             film_times[2]/60, film_times[2]%60);
                    break;
                case 3:
                    snprintf(buffer, sizeof(buffer), "Wash: %d:%02d", 
                             film_times[3]/60, film_times[3]%60);
                    break;
            }
            break;
        case 3:
            switch (cell_index->row) {
                case 0:
                    snprintf(buffer, sizeof(buffer), "Develop: %d:%02d", 
                             rc_print_times[0]/60, rc_print_times[0]%60);
                    break;
                case 1:
                    snprintf(buffer, sizeof(buffer), "Stop: %d:%02d", 
                             rc_print_times[1]/60, rc_print_times[1]%60);
                    break;
                case 2:
                    snprintf(buffer, sizeof(buffer), "Fix: %d:%02d", 
                             rc_print_times[2]/60, rc_print_times[2]%60);
                    break;
                case 3:
                    snprintf(buffer, sizeof(buffer), "Wash: %d:%02d", 
                             rc_print_times[3]/60, rc_print_times[3]%60);
                    break;
            }
            break;
        case 4:
            switch (cell_index->row) {
                case 0:
                    snprintf(buffer, sizeof(buffer), "Develop: %d:%02d",
                             fiber_print_times[0]/60, fiber_print_times[0]%60);
                    break;
                case 1:
                    snprintf(buffer, sizeof(buffer), "Stop: %d:%02d",
                             fiber_print_times[1]/60, fiber_print_times[1]%60);
                    break;
                case 2:
                    snprintf(buffer, sizeof(buffer), "Fix: %d:%02d",
                             fiber_print_times[2]/60, fiber_print_times[2]%60);
                    break;
                case 3:
                    snprintf(buffer, sizeof(buffer), "Wash: %d:%02d",
                             fiber_print_times[3]/60, fiber_print_times[3]%60);
                    break;
                case 4:
                    snprintf(buffer, sizeof(buffer), "HC: %d:%02d",
                             fiber_print_times[4]/60, fiber_print_times[4]%60);
                    break;
                case 5:
                    snprintf(buffer, sizeof(buffer), "Wash2: %d:%02d",
                             fiber_print_times[5]/60, fiber_print_times[5]%60);
                    break;
            }
            break;
    }
    
    // Apply menu color inversion if enabled
    if (s_settings.invert_menu_colors) {
        graphics_context_set_text_color(ctx, GColorWhite);
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, layer_get_bounds(cell_layer), 0, GCornerNone);
    }
    
    menu_cell_basic_draw(ctx, cell_layer, buffer, NULL, NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    // First item in Basic Settings section (row 0) triggers force screen refresh
    if (cell_index->section == 0 && cell_index->row == 0) {
        force_screen_refresh();
        return;
    }
    
    switch (cell_index->section) {
        case 0:
            switch (cell_index->row) {
                case 0:
                    // Force screen refresh handled at the beginning of this function
                    break;
                case 1:
                    s_settings.backlight_enabled = !s_settings.backlight_enabled;
                    light_enable(s_settings.backlight_enabled);
                    break;
            }
            break;
        case 1:
            switch (cell_index->row) {
                case 0:
                    s_settings.invert_timer1_colors = !s_settings.invert_timer1_colors;
                    break;
                case 1:
                    s_settings.invert_timer2_colors = !s_settings.invert_timer2_colors;
                    break;
                case 2:
                    s_settings.invert_menu_colors = !s_settings.invert_menu_colors;
                    break;
            }
            break;
        case 2: {
            // Time adjustment window for film times
            NumberWindow *number_window = number_window_create(
                "Adjust Time (seconds)",
                (NumberWindowCallbacks) {
                    .decremented = NULL,
                    .incremented = NULL,
                    .selected = NULL,
                },
                NULL
            );
            number_window_set_value(number_window, film_times[cell_index->row]);
            window_stack_push(number_window_get_window(number_window), true);
            break;
        }
        case 3: {
            // Time adjustment window for RC print times
            NumberWindow *number_window = number_window_create(
                "Adjust Time (seconds)",
                (NumberWindowCallbacks) {
                    .decremented = NULL,
                    .incremented = NULL,
                    .selected = NULL,
                },
                NULL
            );
            number_window_set_value(number_window, rc_print_times[cell_index->row]);
            window_stack_push(number_window_get_window(number_window), true);
            break;
        }
        case 4: {
            // Time adjustment window for Fiber print times
            NumberWindow *number_window = number_window_create(
                "Adjust Time (seconds)",
                (NumberWindowCallbacks) {
                    .decremented = NULL,
                    .incremented = NULL,
                    .selected = NULL,
                },
                NULL
            );
            
            number_window_set_value(number_window, fiber_print_times[cell_index->row]);
            window_stack_push(number_window_get_window(number_window), true);
            break;
        }
    }
    
    save_settings();
    layer_mark_dirty(menu_layer_get_layer(menu_layer));
    layer_mark_dirty(s_canvas_layer);
}

static void reset_timer(TimerState *timer) {
    if (timer->timer_handle) {
        app_timer_cancel(timer->timer_handle);
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

static void pause_timer(TimerState *timer) {
    if (timer->timer_handle) {
        app_timer_cancel(timer->timer_handle);
        timer->timer_handle = NULL;
    }
    timer->paused = true;
    timer->running = false;
}

static void resume_timer(TimerState *timer) {
    timer->paused = false;
    timer->running = true;
    timer->timer_handle = app_timer_register(1000, timer_callback, timer);
}

// Helper function to get max stage for current timer
static TimerStage get_max_stage(TimerState *timer) {
    if (timer->mode == MODE_FILM) {
        return STAGE_WASH;  // Film has 4 stages ending at WASH
    } else if (timer->paper_type == PAPER_RC) {
        return STAGE_WASH;  // RC paper has 4 stages ending at WASH
    } else {
        return STAGE_WASH2;  // Fiber paper has 6 stages ending at WASH2
    }
}

// Click handlers
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    window_stack_push(s_menu_window, true);
}

// Up button single - reset current timer
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    TimerState *timer = get_active_timer();
    reset_timer(timer);
    update_timer_text();
    update_mode_text();
    layer_mark_dirty(s_canvas_layer);
}

// Up button double - switch between timer 1 and timer 2
static void up_double_click_handler(ClickRecognizerRef recognizer, void *context) {
    s_active_timer = (s_active_timer == 1) ? 2 : 1;
    update_timer_text();
    update_mode_text();
    update_timer_name_text();
    force_screen_refresh();
    layer_mark_dirty(s_canvas_layer);
}

// Up button long - scroll forward through stages in current timer
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    TimerState *timer = get_active_timer();
    TimerStage max_stage = get_max_stage(timer);
    
    // Stop the timer if running
    if (timer->timer_handle) {
        app_timer_cancel(timer->timer_handle);
        timer->timer_handle = NULL;
    }
    timer->running = false;
    timer->paused = false;
    
    // Move to next stage
    if (timer->stage < max_stage) {
        timer->stage++;
    } else {
        // Wrap back to develop
        timer->stage = STAGE_DEVELOP;
    }
    
    // Update timer seconds based on new stage
    int timer_number = (timer == &s_timer1) ? 1 : 2;
    TimerConfig config = get_timer_config(timer_number, timer->mode);
    timer->seconds_remaining = config.timing_array[timer->stage];
    
    update_timer_text();
    update_mode_text();
    layer_mark_dirty(s_canvas_layer);
}

// Down button - control current timer
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    TimerState *timer = get_active_timer();
    
    if (!timer->running && !timer->paused) {
        timer->running = true;
        timer->timer_handle = app_timer_register(1000, timer_callback, timer);
    } else if (timer->running) {
        pause_timer(timer);
    } else if (timer->paused) {
        resume_timer(timer);
    }
    
    update_mode_text();
    layer_mark_dirty(s_canvas_layer);
}

// Down button double - switch between film and print modes
static void down_double_click_handler(ClickRecognizerRef recognizer, void *context) {
    TimerState *timer = get_active_timer();
    timer->mode = (timer->mode == MODE_FILM) ? MODE_PRINT : MODE_FILM;
    
    // When switching to print mode, restore the appropriate paper type for each timer
    if (timer->mode == MODE_PRINT) {
        int timer_number = (timer == &s_timer1) ? 1 : 2;
        TimerConfig config = get_timer_config(timer_number, MODE_PRINT);
        timer->paper_type = config.paper_type;
        timer->max_stages = config.stage_count;
    }
    
    reset_timer(timer);
    update_timer_text();
    update_mode_text();
    layer_mark_dirty(s_canvas_layer);
}

// Down button long - scroll backward through stages in current timer
static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    TimerState *timer = get_active_timer();
    TimerStage max_stage = get_max_stage(timer);
    
    // Stop the timer if running
    if (timer->timer_handle) {
        app_timer_cancel(timer->timer_handle);
        timer->timer_handle = NULL;
    }
    timer->running = false;
    timer->paused = false;
    
    // Move to previous stage
    if (timer->stage > STAGE_DEVELOP) {
        timer->stage--;
    } else {
        // Wrap to max stage
        timer->stage = max_stage;
    }
    
    // Update timer seconds based on new stage
    int timer_number = (timer == &s_timer1) ? 1 : 2;
    TimerConfig config = get_timer_config(timer_number, timer->mode);
    timer->seconds_remaining = config.timing_array[timer->stage];
    
    update_timer_text();
    update_mode_text();
    layer_mark_dirty(s_canvas_layer);
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);

    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);

    window_multi_click_subscribe(BUTTON_ID_UP, 2, 0, 0, true, up_double_click_handler);
    window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 0, 0, true, down_double_click_handler);

    window_long_click_subscribe(BUTTON_ID_UP, 700, up_long_click_handler, NULL);
    window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL);
}

static void main_window_unload(Window *window) {
    layer_destroy(s_canvas_layer);
    text_layer_destroy(s_timer_layer);
    text_layer_destroy(s_mode_layer);
    text_layer_destroy(s_timer_name_layer);
}

static void menu_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    // Create menu layer
    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
        .select_click = menu_select_callback,
    });
    
    // handle menu color inversion
    if (s_settings.invert_menu_colors) {
        window_set_background_color(s_menu_window, GColorBlack);
        menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
        menu_layer_set_highlight_colors(s_menu_layer, GColorWhite, GColorBlack);
    } else {
        window_set_background_color(s_menu_window, GColorWhite);
        menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
        menu_layer_set_highlight_colors(s_menu_layer, GColorBlack, GColorWhite);
    }
    
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void menu_window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
}

// Helper function to get stage display index for proper visual representation
static int get_stage_display_index(TimerStage stage, PaperType paper_type) {
    if (paper_type == PAPER_RC) {
        // RC paper: DEVELOP(0), STOP(1), FIX(2), WASH(3)
        switch (stage) {
            case STAGE_DEVELOP: return 0;
            case STAGE_STOP: return 1;
            case STAGE_FIX: return 2;
            case STAGE_WASH: return 3;
            default: return 0; // Should not happen
        }
    } else {
        // Fiber paper: DEVELOP(0), STOP(1), FIX(2), WASH1(3), HYPO_CLEAR(4), WASH2(5)
        switch (stage) {
            case STAGE_DEVELOP: return 0;
            case STAGE_STOP: return 1;
            case STAGE_FIX: return 2;
            case STAGE_WASH: return 3;
            case STAGE_HYPO_CLEAR: return 4;
            case STAGE_WASH2: return 5;
            default: return 0; // Should not happen
        }
    }
}

// Helper function to draw stage indicators
static void draw_stage_indicators(GContext *ctx, GRect bounds, TimerStage stage, int max_stages, PaperType paper_type) {
    // Calculate indicator width accounting for spacing between indicators
    const int spacing = 2;
    const int total_spacing = (max_stages - 1) * spacing;
    const int available_width = bounds.size.w - total_spacing;
    const int indicator_width = available_width / max_stages;
    const int indicator_height = 3;
    
    // Get the current stage display index
    int current_stage_index = get_stage_display_index(stage, paper_type);
    
    for (int i = 0; i < max_stages; i++) {
        GRect indicator_bounds = GRect(
            bounds.origin.x + (i * (indicator_width + spacing)),
            bounds.origin.y,
            indicator_width,
            indicator_height
        );
        
        // Fill indicators up to and including the current stage
        if (i <= current_stage_index) {
            graphics_fill_rect(ctx, indicator_bounds, 0, GCornerNone);
        } else {
            graphics_draw_rect(ctx, indicator_bounds);
        }
    }
}

// Helper function to get display theme for a timer
typedef struct {
    bool is_light_background;
    GColor text_color;
    GColor background_color;
} DisplayTheme;

static DisplayTheme get_display_theme(int timer_number) {
    bool should_invert = (timer_number == 1) ? 
        s_settings.invert_timer1_colors : s_settings.invert_timer2_colors;
    
    // Timer 1 defaults to light mode, Timer 2 defaults to dark mode
    bool default_light = (timer_number == 1);
    
    // Apply inversion settings to override defaults
    if (should_invert) {
        default_light = !default_light;
    }
    
    return (DisplayTheme){
        .is_light_background = default_light,
        .text_color = default_light ? GColorBlack : GColorWhite,
        .background_color = default_light ? GColorWhite : GColorBlack
    };
}

// Update the canvas drawing procedure
static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    TimerState *timer = get_active_timer();
    
    // Get the appropriate display theme for the active timer
    DisplayTheme theme = get_display_theme(s_active_timer);
    
    // Apply background color to window
    window_set_background_color(s_main_window, theme.background_color);
    
    // Set graphics context colors for drawing stage indicators
    graphics_context_set_fill_color(ctx, theme.text_color);
    graphics_context_set_stroke_color(ctx, theme.text_color);
    
    // Apply colors to all text layers
    text_layer_set_text_color(s_timer_layer, theme.text_color);
    text_layer_set_background_color(s_timer_layer, GColorClear);
    text_layer_set_text_color(s_mode_layer, theme.text_color);
    text_layer_set_background_color(s_mode_layer, GColorClear);
    text_layer_set_text_color(s_timer_name_layer, theme.text_color);
    text_layer_set_background_color(s_timer_name_layer, GColorClear);
    
    // Update text layers
    update_timer_text();
    update_mode_text();
    update_timer_name_text();
    
    // Draw stage indicators at the bottom of the screen
    GRect indicator_bounds = GRect(10, bounds.size.h - 20, bounds.size.w - 20, 3);
    draw_stage_indicators(ctx, indicator_bounds, timer->stage, timer->max_stages, timer->paper_type);
}

// Window load/unload
static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    // Create canvas layer
    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);
    
    // Constants for layout
    const int mode_layer_height = 20;
    const int timer_name_height = 20;
    const int timer_vertical_padding = 10;
    
    // Create timer name layer (at top)
    s_timer_name_layer = text_layer_create(GRect(0, timer_vertical_padding, bounds.size.w, timer_name_height));
    text_layer_set_font(s_timer_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_timer_name_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(s_timer_name_layer));
    
    // Create mode layer (below timer name)
    s_mode_layer = text_layer_create(GRect(0, timer_name_height + timer_vertical_padding*2, 
                                          bounds.size.w, mode_layer_height));
    text_layer_set_font(s_mode_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_mode_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(s_mode_layer));
    
    // Create main timer layer (centered)
    const int timer_top_y = timer_name_height + mode_layer_height + timer_vertical_padding*3;
    const int timer_height = bounds.size.h - timer_top_y - timer_vertical_padding*3;
    
    s_timer_layer = text_layer_create(GRect(0, timer_top_y, bounds.size.w, timer_height));
    text_layer_set_font(s_timer_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(s_timer_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(s_timer_layer));
    
    // Initialize timers with proper paper types
    // Timer 1 defaults to RC paper, Timer 2 defaults to Fiber paper
    s_timer1.mode = MODE_PRINT;
    s_timer1.paper_type = PAPER_RC;
    s_timer1.max_stages = 4;
    
    s_timer2.mode = MODE_PRINT;
    s_timer2.paper_type = PAPER_FIBER;
    s_timer2.max_stages = 6;
    
    reset_timer(&s_timer1);
    reset_timer(&s_timer2);
}

static void init(void) {
    // Load saved settings
    load_settings();
    
    // Create main window
    s_main_window = window_create();
    window_set_click_config_provider(s_main_window, click_config_provider);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload,
    });
    
    // Create menu window
    s_menu_window = window_create();
    window_set_window_handlers(s_menu_window, (WindowHandlers) {
        .load = menu_window_load,
        .unload = menu_window_unload,
    });
    
    // Push main window
    window_stack_push(s_main_window, true);
    
    if (s_settings.backlight_enabled) {
        light_enable(true);
    }
}

static void deinit(void) {
    // Save settings before exit
    save_settings();
    
    // Cleanup windows
    window_destroy(s_main_window);
    window_destroy(s_menu_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
