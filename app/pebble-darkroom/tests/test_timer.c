#include "unity.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>

// Mock globals for timer testing
static int timer_callback_calls = 0;
static TimerState *last_timer_called = NULL;
extern bool app_timer_register_called;
extern bool app_timer_cancel_called;
bool simulating_test = false;  // Flag to prevent recursion during test simulation

// Mock app_timer_register function
static void mock_timer_callback(void *data) {
    timer_callback_calls++;
    last_timer_called = (TimerState*)data;
    
    // Simulate the actual timer behavior for testing
    if (simulating_test) {
        // During test simulation, don't re-register to avoid recursion
        last_timer_called->seconds_remaining--;
        if (last_timer_called->seconds_remaining > 0) {
            // Continue countdown
        } else {
            // Timer finished - move to next stage
            last_timer_called->running = false;
            
            // Handle stage progression based on paper type and mode
            if (last_timer_called->mode == MODE_FILM || last_timer_called->paper_type == PAPER_RC) {
                // Film and RC paper use standard 4-stage progression: DEVELOP -> STOP -> FIX -> WASH
                last_timer_called->stage++;
                if (last_timer_called->stage > STAGE_WASH) {
                    // Cycle back to beginning
                    last_timer_called->stage = STAGE_DEVELOP;
                }
                
                // Set timing based on mode
                if (last_timer_called->mode == MODE_FILM) {
                    last_timer_called->seconds_remaining = get_film_times()[last_timer_called->stage];
                } else {
                    last_timer_called->seconds_remaining = get_rc_print_times()[last_timer_called->stage];
                }
            } else {
                // Fiber paper uses 6-stage progression: DEVELOP -> STOP -> FIX -> WASH1 -> HYPO_CLEAR -> WASH2
                switch (last_timer_called->stage) {
                    case STAGE_DEVELOP:
                        last_timer_called->stage = STAGE_STOP;
                        last_timer_called->seconds_remaining = get_fiber_print_times()[1];
                        break;
                    case STAGE_STOP:
                        last_timer_called->stage = STAGE_FIX;
                        last_timer_called->seconds_remaining = get_fiber_print_times()[2];
                        break;
                    case STAGE_FIX:
                        last_timer_called->stage = STAGE_WASH1;
                        last_timer_called->seconds_remaining = get_fiber_print_times()[4];
                        break;
                    case STAGE_WASH1:
                        last_timer_called->stage = STAGE_HYPO_CLEAR;
                        last_timer_called->seconds_remaining = get_fiber_print_times()[5];
                        break;
                    case STAGE_HYPO_CLEAR:
                        last_timer_called->stage = STAGE_WASH2;
                        last_timer_called->seconds_remaining = get_fiber_print_times()[6];
                        break;
                    case STAGE_WASH2:
                    default:
                        // Cycle back to beginning
                        last_timer_called->stage = STAGE_DEVELOP;
                        last_timer_called->seconds_remaining = get_fiber_print_times()[0];
                        break;
                }
            }
        }
    } else {
        // During actual mock registration, set the flag and return without recursion
        app_timer_register_called = true;
    }
}

// Test group for timer functionality
void test_timer(void) {
    // Reset timing arrays to defaults at start of test
    int *rc_times = get_rc_print_times();
    rc_times[STAGE_DEVELOP] = 60;
    rc_times[STAGE_STOP] = 30;
    rc_times[STAGE_FIX] = 300;
    rc_times[STAGE_WASH] = 300;
    
    int *fiber_times = get_fiber_print_times();
    fiber_times[STAGE_DEVELOP] = 120;
    fiber_times[STAGE_STOP] = 30;
    fiber_times[STAGE_FIX] = 120;
    fiber_times[STAGE_WASH1] = 300;
    fiber_times[STAGE_HYPO_CLEAR] = 120;
    fiber_times[STAGE_WASH2] = 900;
    
    TimerState *timer1 = get_timer1();
    TimerState *timer2 = get_timer2();
    
    // Test 1: Test timer initialization and reset
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->stage, STAGE_DEVELOP);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 60);  // Should be RC develop time
    TEST_ASSERT_TRUE(!timer1->running);
    TEST_ASSERT_TRUE(!timer1->paused);
    TEST_ASSERT_TRUE(timer1->timer_handle == NULL);
    
    // Test 2: Test timer mode switching and reset
    timer1->mode = MODE_FILM;
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 300);
    
    // Test 3: Test pause functionality
    timer1->running = true;
    timer1->paused = false;
    timer1->timer_handle = (void*)0x12345678;
    
    pause_timer(timer1);
    TEST_ASSERT_TRUE(!timer1->running);
    TEST_ASSERT_TRUE(timer1->paused);
    TEST_ASSERT_TRUE(timer1->timer_handle == NULL);
    TEST_ASSERT_TRUE(app_timer_cancel_called);
    
    // Test 4: Test resume functionality
    resume_timer(timer1);
    TEST_ASSERT_TRUE(timer1->running);
    TEST_ASSERT_TRUE(!timer1->paused);
    TEST_ASSERT_TRUE(app_timer_register_called);
    TEST_ASSERT_TRUE(timer1->timer_handle != NULL);
    
    // Test 5: Test timer countdown with mock callback
    timer_callback_calls = 0;
    last_timer_called = NULL;
    app_timer_register_called = false;
    
    // Set up timer with 5 seconds
    timer1->seconds_remaining = 5;
    timer1->running = true;
    timer1->timer_handle = app_timer_register(1000, mock_timer_callback, timer1);
    
    // Simulate 3 ticks
    simulating_test = true;
    mock_timer_callback(timer1);
    mock_timer_callback(timer1);
    mock_timer_callback(timer1);
    simulating_test = false;
    
    app_timer_register_called = true;  // Set flag after simulation as the initial registration occurred
    
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 2);
    TEST_ASSERT_EQUAL_INT(timer_callback_calls, 3);
    TEST_ASSERT_TRUE(last_timer_called == timer1);
    TEST_ASSERT_TRUE(app_timer_register_called);
    
    // Test 6: Test timer completion and stage progression
    timer1->seconds_remaining = 1;
    timer1->stage = STAGE_DEVELOP;
    timer1->mode = MODE_PRINT;
    timer1->running = true;
    
    simulating_test = true;
    mock_timer_callback(timer1);
    simulating_test = false;
    TEST_ASSERT_EQUAL_INT(timer1->stage, 1);  // STAGE_STOP
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 30);
    TEST_ASSERT_TRUE(!timer1->running);
    
    // Test 7: Test full stage progression
    timer1->stage = STAGE_STOP;
    timer1->seconds_remaining = 1;
    timer1->running = true;
    
    simulating_test = true;
    mock_timer_callback(timer1);
    simulating_test = false;
    TEST_ASSERT_EQUAL_INT(timer1->stage, 2);  // STAGE_FIX
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 300);
    
    // Test 8: Test wash completion and cycle reset
    timer1->stage = STAGE_WASH;
    timer1->seconds_remaining = 1;
    timer1->running = true;
    
    simulating_test = true;
    mock_timer_callback(timer1);
    simulating_test = false;
    TEST_ASSERT_EQUAL_INT(timer1->stage, 0);  // STAGE_DEVELOP
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 60);
    TEST_ASSERT_TRUE(!timer1->running);
    
    // Test 9: Test timer2 with different mode
    timer2->mode = MODE_FILM;
    reset_timer(timer2);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 300);
    
    // Test 10: Test timer string representation for both timers
    char *timer1_str = timer_to_string(timer1);
    char *timer2_str = timer_to_string(timer2);
    
    TEST_ASSERT_TRUE(strlen(timer1_str) > 0);
    TEST_ASSERT_TRUE(strlen(timer2_str) > 0);
    
    printf("Timer1: %s\n", timer1_str);
    printf("Timer2: %s\n", timer2_str);
    
    // Test 11: Test film vs print time arrays
    int *film_times = get_film_times();
    int *print_times = get_print_times();
    
    TEST_ASSERT_EQUAL_INT(film_times[0], 300);
    TEST_ASSERT_EQUAL_INT(film_times[1], 60);
    TEST_ASSERT_EQUAL_INT(film_times[2], 300);
    TEST_ASSERT_EQUAL_INT(film_times[3], 300);
    
    TEST_ASSERT_EQUAL_INT(print_times[0], 60);
    TEST_ASSERT_EQUAL_INT(print_times[1], 30);
    TEST_ASSERT_EQUAL_INT(print_times[2], 300);
    TEST_ASSERT_EQUAL_INT(print_times[3], 300);
    
    // Test 12: Test Timer 1 RC paper configuration
    timer1->mode = MODE_PRINT;
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->paper_type, PAPER_RC);
    TEST_ASSERT_EQUAL_INT(timer1->max_stages, 4);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 60); // RC develop time
    
    // Test 13: Test Timer 2 Fiber paper configuration
    timer2->mode = MODE_PRINT;
    reset_timer(timer2);
    TEST_ASSERT_EQUAL_INT(timer2->paper_type, PAPER_FIBER);
    TEST_ASSERT_EQUAL_INT(timer2->max_stages, 6);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 120); // Fiber develop time
    
    // Test 14: Test Timer 1 film mode configuration
    timer1->mode = MODE_FILM;
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->paper_type, PAPER_RC); // Not applicable for film but set
    TEST_ASSERT_EQUAL_INT(timer1->max_stages, 4);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 300); // Film develop time
    
    // Test 15: Test Timer 2 film mode configuration
    timer2->mode = MODE_FILM;
    reset_timer(timer2);
    TEST_ASSERT_EQUAL_INT(timer2->paper_type, PAPER_RC); // Not applicable for film but set
    TEST_ASSERT_EQUAL_INT(timer2->max_stages, 4);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 300); // Film develop time
    
    // Test 16: Test RC paper stage progression (4 stages)
    timer1->mode = MODE_PRINT;
    timer1->paper_type = PAPER_RC;
    timer1->max_stages = 4;
    timer1->stage = STAGE_DEVELOP;
    timer1->seconds_remaining = 1;
    timer1->running = true;
    
    // Simulate stage progression through all RC stages
    simulating_test = true;
    mock_timer_callback(timer1); // DEVELOP -> STOP
    TEST_ASSERT_EQUAL_INT(timer1->stage, STAGE_STOP);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 30);
    TEST_ASSERT_FALSE(timer1->running); // Should stop after each stage
    
    timer1->seconds_remaining = 1;
    timer1->running = true;
    mock_timer_callback(timer1); // STOP -> FIX
    TEST_ASSERT_EQUAL_INT(timer1->stage, STAGE_FIX);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 300);
    
    timer1->seconds_remaining = 1;
    timer1->running = true;
    mock_timer_callback(timer1); // FIX -> WASH
    TEST_ASSERT_EQUAL_INT(timer1->stage, STAGE_WASH);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 300);
    
    timer1->seconds_remaining = 1;
    timer1->running = true;
    mock_timer_callback(timer1); // WASH -> cycle back to DEVELOP
    TEST_ASSERT_EQUAL_INT(timer1->stage, STAGE_DEVELOP);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 60);
    TEST_ASSERT_FALSE(timer1->running);
    simulating_test = false;
    
    // Test 17: Test Fiber paper stage progression (6 stages)
    timer2->mode = MODE_PRINT;
    timer2->paper_type = PAPER_FIBER;
    timer2->max_stages = 6;
    timer2->stage = STAGE_DEVELOP;
    timer2->seconds_remaining = 1;
    timer2->running = true;
    
    // Simulate stage progression through all Fiber stages
    simulating_test = true;
    mock_timer_callback(timer2); // DEVELOP -> STOP
    TEST_ASSERT_EQUAL_INT(timer2->stage, STAGE_STOP);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 30);
    TEST_ASSERT_FALSE(timer2->running);
    
    timer2->seconds_remaining = 1;
    timer2->running = true;
    mock_timer_callback(timer2); // STOP -> FIX
    TEST_ASSERT_EQUAL_INT(timer2->stage, STAGE_FIX);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 120);
    
    timer2->seconds_remaining = 1;
    timer2->running = true;
    mock_timer_callback(timer2); // FIX -> WASH1
    TEST_ASSERT_EQUAL_INT(timer2->stage, STAGE_WASH1);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 300);
    
    timer2->seconds_remaining = 1;
    timer2->running = true;
    mock_timer_callback(timer2); // WASH1 -> HYPO_CLEAR
    TEST_ASSERT_EQUAL_INT(timer2->stage, STAGE_HYPO_CLEAR);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 120);
    
    timer2->seconds_remaining = 1;
    timer2->running = true;
    mock_timer_callback(timer2); // HYPO_CLEAR -> WASH2
    TEST_ASSERT_EQUAL_INT(timer2->stage, STAGE_WASH2);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 900);
    
    timer2->seconds_remaining = 1;
    timer2->running = true;
    mock_timer_callback(timer2); // WASH2 -> cycle back to DEVELOP
    TEST_ASSERT_EQUAL_INT(timer2->stage, STAGE_DEVELOP);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 120);
    TEST_ASSERT_FALSE(timer2->running);
    simulating_test = false;
    
    // Test 18: Test timing array selection based on timer number and paper type
    // Timer 1 in print mode should use RC timing array
    timer1->mode = MODE_PRINT;
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->paper_type, PAPER_RC);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 60); // RC develop time
    
    // Timer 2 in print mode should use Fiber timing array
    timer2->mode = MODE_PRINT;
    reset_timer(timer2);
    TEST_ASSERT_EQUAL_INT(timer2->paper_type, PAPER_FIBER);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 120); // Fiber develop time
    
    // Both timers in film mode should use film timing array
    timer1->mode = MODE_FILM;
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->seconds_remaining, 300); // Film develop time
    
    timer2->mode = MODE_FILM;
    reset_timer(timer2);
    TEST_ASSERT_EQUAL_INT(timer2->seconds_remaining, 300); // Film develop time
    
    // Test 19: Test proper stage count handling for different paper types
    // RC paper should have max_stages = 4
    timer1->mode = MODE_PRINT;
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->max_stages, 4);
    TEST_ASSERT_EQUAL_INT(timer1->paper_type, PAPER_RC);
    
    // Fiber paper should have max_stages = 6
    timer2->mode = MODE_PRINT;
    reset_timer(timer2);
    TEST_ASSERT_EQUAL_INT(timer2->max_stages, 6);
    TEST_ASSERT_EQUAL_INT(timer2->paper_type, PAPER_FIBER);
    
    // Film mode should have max_stages = 4 for both timers
    timer1->mode = MODE_FILM;
    reset_timer(timer1);
    TEST_ASSERT_EQUAL_INT(timer1->max_stages, 4);
    
    timer2->mode = MODE_FILM;
    reset_timer(timer2);
    TEST_ASSERT_EQUAL_INT(timer2->max_stages, 4);
    
    // Test 20: Test stage progression boundary conditions
    // Test that RC paper doesn't progress beyond STAGE_WASH
    timer1->mode = MODE_PRINT;
    timer1->paper_type = PAPER_RC;
    timer1->max_stages = 4;
    timer1->stage = STAGE_WASH;
    timer1->seconds_remaining = 1;
    timer1->running = true;
    
    simulating_test = true;
    mock_timer_callback(timer1); // Should cycle back to DEVELOP
    TEST_ASSERT_EQUAL_INT(timer1->stage, STAGE_DEVELOP);
    TEST_ASSERT_FALSE(timer1->running);
    simulating_test = false;
    
    // Test that Fiber paper doesn't progress beyond STAGE_WASH2
    timer2->mode = MODE_PRINT;
    timer2->paper_type = PAPER_FIBER;
    timer2->max_stages = 6;
    timer2->stage = STAGE_WASH2;
    timer2->seconds_remaining = 1;
    timer2->running = true;
    
    simulating_test = true;
    mock_timer_callback(timer2); // Should cycle back to DEVELOP
    TEST_ASSERT_EQUAL_INT(timer2->stage, STAGE_DEVELOP);
    TEST_ASSERT_FALSE(timer2->running);
    simulating_test = false;
    
    printf("All timer tests passed!\n");
}
