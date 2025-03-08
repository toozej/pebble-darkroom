#include <pebble.h>

#define SETTINGS_KEY 1
#define FILM_TIMES_KEY 2
#define PRINT_TIMES_KEY 3

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
    STAGE_WASH
} TimerStage;

typedef struct {
    bool running;
    bool paused;
    TimerMode mode;
    TimerStage stage;
    int seconds_remaining;
    AppTimer *timer_handle;
} TimerState;

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
    .invert_timer1_colors = false,  // Timer 1 is white bg by default
    .invert_timer2_colors = false,  // Timer 2 is black bg by default
    .invert_menu_colors = false
};

// Timer settings (in seconds)
static int film_times[4] = {
    300,  // Develop: 5 mins
    60,   // Stop: 1 min
    300,  // Fix: 5 mins
    300   // Wash: 5 mins
};

static int print_times[4] = {
    60,   // Develop: 1 min
    30,   // Stop: 30 secs
    300,  // Fix: 5 mins
    300   // Wash: 5 mins
};

// Persistent storage functions
static void save_settings() {
    persist_write_data(SETTINGS_KEY, &s_settings, sizeof(Settings));
    persist_write_data(FILM_TIMES_KEY, &film_times, sizeof(film_times));
    persist_write_data(PRINT_TIMES_KEY, &print_times, sizeof(print_times));
}

static void load_settings() {
    if (persist_exists(SETTINGS_KEY)) {
        persist_read_data(SETTINGS_KEY, &s_settings, sizeof(Settings));
    }
    if (persist_exists(FILM_TIMES_KEY)) {
        persist_read_data(FILM_TIMES_KEY, &film_times, sizeof(film_times));
    }
    if (persist_exists(PRINT_TIMES_KEY)) {
        persist_read_data(PRINT_TIMES_KEY, &print_times, sizeof(print_times));
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
    static char s_buffer[20];
    char mode_char = (timer->mode == MODE_FILM) ? 'F' : 'P';
    const char *stage_text = "Unknown";
    switch (timer->stage) {
        case STAGE_DEVELOP: stage_text = "Dev"; break;
        case STAGE_STOP: stage_text = "Stop"; break;
        case STAGE_FIX: stage_text = "Fix"; break;
        case STAGE_WASH: stage_text = "Wash"; break;
    }
    snprintf(s_buffer, sizeof(s_buffer), "%c | %s | %s", 
             mode_char, stage_text, 
             timer->paused ? "PAUSED" : "");
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
        if (timer->stage > STAGE_WASH) {
            timer->stage = STAGE_DEVELOP;
            timer->running = false;
        } else {
            // Set the time for the next stage but don't start the timer
            timer->seconds_remaining = (timer->mode == MODE_FILM) ? 
                film_times[timer->stage] : print_times[timer->stage];
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

// Menu callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 4;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0: return 2;  // Basic Settings
        case 1: return 3;  // Color Settings
        case 2: return 4;  // Film times
        case 3: return 4;  // Print times
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
            menu_cell_basic_header_draw(ctx, cell_layer, "Print Times");
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
                             print_times[0]/60, print_times[0]%60);
                    break;
                case 1:
                    snprintf(buffer, sizeof(buffer), "Stop: %d:%02d", 
                             print_times[1]/60, print_times[1]%60);
                    break;
                case 2:
                    snprintf(buffer, sizeof(buffer), "Fix: %d:%02d", 
                             print_times[2]/60, print_times[2]%60);
                    break;
                case 3:
                    snprintf(buffer, sizeof(buffer), "Wash: %d:%02d", 
                             print_times[3]/60, print_times[3]%60);
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
    switch (cell_index->section) {
        case 0:
            switch (cell_index->row) {
                case 0:
                    s_settings.vibration_enabled = !s_settings.vibration_enabled;
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
            int *film_time_array = (cell_index->section == 1) ? film_times : print_times;
            NumberWindow *number_window = number_window_create(
                "Adjust Time (seconds)",
                (NumberWindowCallbacks) {
                    .decremented = NULL,
                    .incremented = NULL,
                    .selected = NULL,
                },
                NULL
            );
            number_window_set_value(number_window, film_time_array[cell_index->row]);
            window_stack_push(number_window_get_window(number_window), true);
            break;
        }
        case 3: {
            // Time adjustment window for print times
            int *print_time_array = (cell_index->section == 1) ? print_times : print_times;
            NumberWindow *number_window = number_window_create(
                "Adjust Time (seconds)",
                (NumberWindowCallbacks) {
                    .decremented = NULL,
                    .incremented = NULL,
                    .selected = NULL,
                },
                NULL
            );
            number_window_set_value(number_window, print_time_array[cell_index->row]);
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
    timer->seconds_remaining = (timer->mode == MODE_FILM) ? 
        film_times[STAGE_DEVELOP] : print_times[STAGE_DEVELOP];
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

static void force_screen_refresh() {
    // Force a complete layer refresh to address screen tearing
    layer_mark_dirty(window_get_root_layer(s_main_window));
    
    // Optionally provide haptic feedback to confirm the action
    if (s_settings.vibration_enabled) {
        vibes_short_pulse();
    }
}

// Click handlers
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    window_stack_push(s_menu_window, true);
}

// Up button long - force screen refresh
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    force_screen_refresh();
}

// Up button - switch between timer 1 and timer 2
static void up_double_click_handler(ClickRecognizerRef recognizer, void *context) {
    s_active_timer = (s_active_timer == 1) ? 2 : 1;
    update_timer_text();
    update_mode_text();
    update_timer_name_text();
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

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    TimerState *timer = get_active_timer();
    reset_timer(timer);
    update_timer_text();
    update_mode_text();
    layer_mark_dirty(s_canvas_layer);
}

static void down_double_click_handler(ClickRecognizerRef recognizer, void *context) {
    TimerState *timer = get_active_timer();
    timer->mode = (timer->mode == MODE_FILM) ? MODE_PRINT : MODE_FILM;
    reset_timer(timer);
    update_timer_text();
    update_mode_text();
    layer_mark_dirty(s_canvas_layer);
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    
    window_long_click_subscribe(BUTTON_ID_UP, 700, up_long_click_handler, NULL);
    window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL);
    
    window_multi_click_subscribe(BUTTON_ID_UP, 2, 0, 0, true, up_double_click_handler);
    window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 0, 0, true, down_double_click_handler);
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

// Helper function to draw stage indicators
static void draw_stage_indicators(GContext *ctx, GRect bounds, TimerStage stage) {
    const int indicator_width = bounds.size.w / 4;
    const int indicator_height = 3;
    const int spacing = 2;
    
    for (int i = 0; i < 4; i++) {
        GRect indicator_bounds = GRect(
            bounds.origin.x + (i * (indicator_width + spacing)),
            bounds.origin.y,
            indicator_width,
            indicator_height
        );
        
        if (i <= stage) {
            graphics_fill_rect(ctx, indicator_bounds, 0, 0);
        } else {
            graphics_draw_rect(ctx, indicator_bounds);
        }
    }
}

// Update the canvas drawing procedure
static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    TimerState *timer = get_active_timer();
    bool should_invert = (s_active_timer == 1) ? s_settings.invert_timer1_colors : s_settings.invert_timer2_colors;
    
    // Default colors based on which timer is active
    bool is_light_bg = (s_active_timer == 1);
    
    // Apply inversion settings
    if (should_invert) {
        is_light_bg = !is_light_bg;
    }
    
    // Set background color
    if (is_light_bg) {
        // White background with black text
        window_set_background_color(s_main_window, GColorWhite);
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        text_layer_set_text_color(s_timer_layer, GColorBlack);
        text_layer_set_background_color(s_timer_layer, GColorClear);
        text_layer_set_text_color(s_mode_layer, GColorBlack);
        text_layer_set_background_color(s_mode_layer, GColorClear);
        text_layer_set_text_color(s_timer_name_layer, GColorBlack);
        text_layer_set_background_color(s_timer_name_layer, GColorClear);
    } else {
        // Black background with white text
        window_set_background_color(s_main_window, GColorBlack);
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        text_layer_set_text_color(s_timer_layer, GColorWhite);
        text_layer_set_background_color(s_timer_layer, GColorClear);
        text_layer_set_text_color(s_mode_layer, GColorWhite);
        text_layer_set_background_color(s_mode_layer, GColorClear);
        text_layer_set_text_color(s_timer_name_layer, GColorWhite);
        text_layer_set_background_color(s_timer_name_layer, GColorClear);
    }
    
    // Update text layers
    update_timer_text();
    update_mode_text();
    update_timer_name_text();
    
    // Draw stage indicators at the bottom of the screen
    GRect indicator_bounds = GRect(10, bounds.size.h - 20, bounds.size.w - 20, 3);
    draw_stage_indicators(ctx, indicator_bounds, timer->stage);
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
    text_layer_set_font(s_mode_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(s_mode_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(s_mode_layer));
    
    // Create main timer layer (centered)
    const int timer_top_y = timer_name_height + mode_layer_height + timer_vertical_padding*3;
    const int timer_height = bounds.size.h - timer_top_y - timer_vertical_padding*3;
    
    s_timer_layer = text_layer_create(GRect(0, timer_top_y, bounds.size.w, timer_height));
    text_layer_set_font(s_timer_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(s_timer_layer, GTextAlignmentCenter);
    layer_add_child(s_canvas_layer, text_layer_get_layer(s_timer_layer));
    
    // Initialize timers
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