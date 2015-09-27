// Set length of long click for emulator or phone
#define LONG_CLICK_LENGTH 2000		// 2000 for emulator, 0 for watch

#include <pebble.h>
#include <limits.h>
#include "timer_window.h"

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_gothic_24_bold;
static GFont s_res_gothic_28_bold;
static GFont s_res_roboto_bold_subset_49;
static GFont s_res_gothic_18_bold;
static TextLayer *s_clock_layer;
static TextLayer *s_count_layer;
static TextLayer *s_timer_layer;
static TextLayer *s_mode_layer;

static void initialise_ui(void) {
  s_window = window_create();
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, 0);
  #endif
  
  s_res_gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_roboto_bold_subset_49 = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  // s_clock_layer
  s_clock_layer = text_layer_create(GRect(8, 9, 130, 31));
  text_layer_set_text(s_clock_layer, "00:00:00 AM");
  text_layer_set_text_alignment(s_clock_layer, GTextAlignmentCenter);
  text_layer_set_font(s_clock_layer, s_res_gothic_24_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_clock_layer);
  
  // s_count_layer
  s_count_layer = text_layer_create(GRect(5, 66, 134, 38));
  text_layer_set_text(s_count_layer, "0:00:00");
  text_layer_set_text_alignment(s_count_layer, GTextAlignmentCenter);
  text_layer_set_font(s_count_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_count_layer);
  
  // s_timer_layer
  s_timer_layer = text_layer_create(GRect(5, 52, 135, 63));
  text_layer_set_text(s_timer_layer, "5:00");
  text_layer_set_text_alignment(s_timer_layer, GTextAlignmentCenter);
  text_layer_set_font(s_timer_layer, s_res_roboto_bold_subset_49);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_timer_layer);
  
  // s_mode_layer
  s_mode_layer = text_layer_create(GRect(7, 122, 131, 23));
  text_layer_set_text(s_mode_layer, "Rolling");
  text_layer_set_text_alignment(s_mode_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mode_layer, s_res_gothic_18_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_mode_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(s_clock_layer);
  text_layer_destroy(s_count_layer);
  text_layer_destroy(s_timer_layer);
  text_layer_destroy(s_mode_layer);
}
// END AUTO-GENERATED UI CODE

// Persistent storage key(s)
#define PERSIST_TIMER_MODE 1
#define PERSIST_START_TIME 2

// Timer variables
time_t start_time = 0;					    	// start time (set when timer starts)
static int timer_initial = -(5 * 60);	// initial timer values in seconds
static int timer_value = -(5 * 60);		// current timer values in seconds
const int MARGIN = 2;         		  	// time margin in seconds when multi-pressing up button
static bool timer_run = false;      	// TRUE if timer is running
static bool timer_sync = false;				// TRUE to sync timer to nearest clock minute
// what timer does when zero is reached
static enum TIMER_MODE {TIMER_STOP, TIMER_ROLLING, TIMER_COUNT} timer_mode = TIMER_ROLLING;

// Display timer value
static void display_timer() {
  // Create a long-lived buffer
  static char timer_buffer[] = "00:00:00";
	struct tm timer_time;
  
	// set timer text & background color
	if (timer_value > 0) {													// timer counting up
		timer_time.tm_hour = timer_value / 3660;
		timer_value %= 3600;
		timer_time.tm_min = timer_value / 60;
		timer_time.tm_sec = timer_value % 60;
		strftime(timer_buffer, sizeof("00:00:00"), "%k:%M:%S", &timer_time);
	  text_layer_set_text(s_count_layer, timer_buffer);		// timer text
		text_layer_set_background_color(s_count_layer, timer_run ? GColorGreen : GColorWhite);
		layer_set_hidden((Layer*)s_count_layer, false);
		layer_set_hidden((Layer*)s_timer_layer, true);
	} else {																				// timer counting down
		// use hour:min so leading 0 is suppressed
		timer_time.tm_hour = -timer_value / 60;
		timer_time.tm_min = -timer_value % 60;
		strftime(timer_buffer, sizeof("00:00"), "%k:%M", &timer_time);
	  text_layer_set_text(s_timer_layer, timer_buffer);	// timer text
		text_layer_set_background_color(s_timer_layer, timer_run ?
			(timer_value < (-1 * 60) ? GColorYellow : GColorOrange) :
			GColorWhite);
		layer_set_hidden((Layer*)s_count_layer, true);
		layer_set_hidden((Layer*)s_timer_layer, false);
	}
}

// Display timer mode
static void display_mode() {
	switch (timer_mode) {
	case TIMER_STOP:
		text_layer_set_text(s_mode_layer, "Down-Stop");
		break;
	case TIMER_ROLLING:
		text_layer_set_text(s_mode_layer, "Rolling");
		break;
	case TIMER_COUNT:
		text_layer_set_text(s_mode_layer, "Down-Up");
		break;
	}
}

static void update_clock_timer() {

	// UPDATE CLOCK
	
	// Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char clock_buffer[] = "00:00:00 AM";

  // Write the current time into the buffer
  if(clock_is_24h_style()) {				// 24 hour format
    strftime(clock_buffer, sizeof("00:00:00"), "%H:%M:%S", tick_time);
  } else {													// 12 hour format
    strftime(clock_buffer, sizeof("00:00:00 AM"), "%I:%M:%S %p", tick_time);
  }
  // Display this time on the TextLayer
  text_layer_set_text(s_clock_layer, clock_buffer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Clock: %s", clock_buffer);

	// USE START TIME IF I HAVE IT, OTHERWISE INCREMENT
	
	if (timer_run) {
		if (start_time) {
			timer_value = temp - start_time;
		} else {
			++timer_value;
		}	
	}
	
	// HANDLE SYNC
	
	if (timer_sync) {
		int offset = timer_value % 60 - (60 - tick_time->tm_sec);
		timer_value -= (offset <= 30) ? offset : offset - 60;
		timer_sync = false;
		start_time = 0;							// sync start time
	} 
	
	// UPDATE TIMER
	
  if (timer_run) {
		// VIBES
		switch (timer_value) {			// vibration test
			case -4 * 60:
			case -10: case -9: case -8: case -7: case -6: case -5: case -4: case -3: case -2: case -1: 
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Short Vibe");
			vibes_short_pulse();
			break;
			case -1 * 60:
			case 0:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Long Vibe");
			vibes_long_pulse();
			break;
			default:
			break;
		}
		// LOGIC
		if (timer_value >= 0) {			// start or after
			switch (timer_mode) {
			case TIMER_STOP:					// count down and STOP
				timer_value = 0;
				timer_run = false;
				break;
			case TIMER_ROLLING:				// ROLLING starts
				timer_value = timer_initial;
				start_time -= timer_initial;
				break;
			case TIMER_COUNT:					// count down then count UP
				break;
			}
		}
	  display_timer();
  }

	// CAPTURE START TIME
	
	if (timer_run && ! start_time) {
		start_time = temp - timer_value;
		persist_write_int(PERSIST_START_TIME, (int32_t) start_time);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved Start time: %d", (int) start_time);
	}
}

// CLICK HANDLERS

// Up click jumps back to minute
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Up click handled");
	int secs = timer_value % 60;
	int correct = ((60 - (secs % 60)) % 60) - 60;
	timer_value += correct < -MARGIN ? correct : correct - 60 ;
  timer_sync = false;
  display_timer();
	start_time = 0;									// sync start time
}

// Up long click resets timer
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Up long click handled");
  timer_run = false;
  timer_sync = false;
	timer_value = timer_initial;
  display_timer();
	start_time = 0;									// sync start time
}

// Select click toggles start/stop
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Select click handled");
	if ((timer_run = ! timer_run) && timer_mode != TIMER_COUNT && timer_value >= 0) {
		timer_value = timer_initial;
	}
	timer_sync = false;
	display_timer();
	start_time = 0;									// sync start time
}

// Select long click syncs minute to closest clock minute
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Select long click handled");
	timer_sync = true;							// sync on next tick
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Double Vibe");
	vibes_double_pulse();
}

// Down click jumps forward to minute
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Down click handled");
	timer_value += (60 - (timer_value % 60)) % 60;
	if (timer_mode != TIMER_COUNT && timer_value > 0) {
		timer_value = 0;
	}
  timer_sync = false;
  display_timer();
	start_time = 0;									// sync start time
}

// Down long click rotates timer mode
static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Down long click handled");
	switch (timer_mode) {
	case TIMER_ROLLING:
		timer_mode = TIMER_COUNT;
		break;
	case TIMER_COUNT:
		timer_mode = TIMER_STOP;
		break;
	default:
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer Mode error");
	case TIMER_STOP:
		timer_mode = TIMER_ROLLING;
		break;
	}
	display_mode();
	persist_write_int(PERSIST_TIMER_MODE, (int) timer_mode);
}

// INITIALIZATION

static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, LONG_CLICK_LENGTH, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, LONG_CLICK_LENGTH, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, LONG_CLICK_LENGTH, down_long_click_handler, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_clock_timer();
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_timer_window(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  // Register button click handlers
  window_set_click_config_provider(s_window, click_config_provider);
	// retrieve timer mode and make sure it's good
	timer_mode = persist_exists(PERSIST_TIMER_MODE) ?
		(enum TIMER_MODE) persist_read_int(PERSIST_TIMER_MODE) :
		TIMER_ROLLING ;
	switch (timer_mode) {
	case TIMER_STOP: case TIMER_ROLLING: case TIMER_COUNT:
		break;
	default:
		timer_mode = TIMER_ROLLING;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Retrieved Timer Mode fixed");
		break;
	}
	// retrieve and check start time
	start_time = persist_exists(PERSIST_START_TIME) ?
		(int) persist_read_int(PERSIST_START_TIME) :
		0 ;
	{
	  time_t temp = time(NULL); 
		if (start_time && (start_time > temp || start_time < temp - 99 * 60 * 60)) {
			start_time = 0;
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Retrieved Start time fixed");
		}
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Retrieved Start time: %d", (int) start_time);
	// restart if start time
	if (start_time) {
		timer_run = true;
	}
	display_timer();
	display_mode();
}

void hide_timer_window(void) {
  window_stack_remove(s_window, true);
	// clear start time if timer not running
	if (! timer_run && start_time) {
		persist_write_int(PERSIST_START_TIME, 0);
	}
}
