#include <pebble.h>
#include <limits.h>
#include "timer_window.h"

// Platform compatibility
static GColor MyYellow, MyOrange, MyGreen;

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_gothic_28_bold;
static GFont s_res_gothic_24_bold;
static GFont s_res_font_roboto_bold_54;
static GFont s_res_font_robotocondensed_bold_37;
static TextLayer *s_clock_layer;
static TextLayer *s_start_layer;
static TextLayer *s_mode_layer;
static TextLayer *s_timer_layer;
static TextLayer *s_count_layer;

static void initialise_ui(void) {
  s_window = window_create();
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, 0);
  #endif
  
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_gothic_24_bold = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_res_font_roboto_bold_54 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_54));
  s_res_font_robotocondensed_bold_37 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTOCONDENSED_BOLD_37));
  // s_clock_layer
  s_clock_layer = text_layer_create(GRect(8, 0, 130, 31));
  text_layer_set_text(s_clock_layer, " 00:00:00");
  text_layer_set_text_alignment(s_clock_layer, GTextAlignmentCenter);
  text_layer_set_font(s_clock_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_clock_layer);
  
  // s_start_layer
  s_start_layer = text_layer_create(GRect(2, 129, 140, 28));
  text_layer_set_text(s_start_layer, "Start 00:00:00 AM");
  text_layer_set_text_alignment(s_start_layer, GTextAlignmentCenter);
  text_layer_set_font(s_start_layer, s_res_gothic_24_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_start_layer);
  
  // s_mode_layer
  s_mode_layer = text_layer_create(GRect(8, 101, 130, 32));
  text_layer_set_text(s_mode_layer, " DOWN-UP");
  text_layer_set_text_alignment(s_mode_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mode_layer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_mode_layer);
  
  // s_timer_layer
  s_timer_layer = text_layer_create(GRect(1, 36, 142, 70));
  text_layer_set_text(s_timer_layer, "Text layer");
  text_layer_set_text_alignment(s_timer_layer, GTextAlignmentCenter);
  text_layer_set_font(s_timer_layer, s_res_font_roboto_bold_54);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_timer_layer);
  
  // s_count_layer
  s_count_layer = text_layer_create(GRect(2, 49, 141, 46));
  text_layer_set_text(s_count_layer, "00:00:00");
  text_layer_set_text_alignment(s_count_layer, GTextAlignmentCenter);
  text_layer_set_font(s_count_layer, s_res_font_robotocondensed_bold_37);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_count_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(s_clock_layer);
  text_layer_destroy(s_start_layer);
  text_layer_destroy(s_mode_layer);
  text_layer_destroy(s_timer_layer);
  text_layer_destroy(s_count_layer);
  fonts_unload_custom_font(s_res_font_roboto_bold_54);
  fonts_unload_custom_font(s_res_font_robotocondensed_bold_37);
}
// END AUTO-GENERATED UI CODE

// Persistent storage key(s)
#define PERSIST_TIMER_MODE 1
#define PERSIST_START_TIME 2

// Timer variables
time_t start_time = 0;					    	// start time (set when timer starts)
static int timer_initial = -(5 * 60);	// initial timer values in seconds
static int timer_value = 0;						// current timer values in seconds
const int MARGIN = 2;         		  	// time margin in seconds when multi-pressing up button
static bool timer_run = false;      	// TRUE if timer is running
static bool timer_sync = false;				// TRUE to sync timer to nearest clock minute
// what timer does when zero is reached
static enum TIMER_MODE {TIMER_STOP, TIMER_ROLLING, TIMER_COUNT} timer_mode = TIMER_COUNT;

// Set start time (memory and persistent)
static void set_start_time(time_t value) {
	start_time = value;
	persist_write_int(PERSIST_START_TIME, (int32_t) start_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Start time set to %d", (int) start_time);
	wakeup_cancel_all();								// cancel any pending wakeups
	if (start_time) {										// wakeup for 10 second countdown
		WakeupId id = wakeup_schedule(start_time - 10, 0, false);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "WakeupId: %d", (int) id);
	}
	
  // Display start time
	if (start_time) {
	  // Create a long-lived buffer
  	static char start_buffer[] = "Start 00:00:00 AM";
		struct tm *start_time_tm = localtime(&start_time);
		if(clock_is_24h_style()) {				// 24 hour format
			strftime(start_buffer, sizeof("Start 00:00:00"), "Start %H:%M:%S", start_time_tm);
		} else {													// 12 hour format
			strftime(start_buffer, sizeof("Start 00:00:00 AM"), "Start %l:%M:%S %p", start_time_tm);
		}
	  text_layer_set_text(s_start_layer, start_buffer);
	} else {
		text_layer_set_text(s_start_layer, "");
	}
}

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
		text_layer_set_background_color(s_count_layer, timer_run ? MyGreen : GColorWhite);
		layer_set_hidden((Layer*)s_count_layer, false);
		layer_set_hidden((Layer*)s_timer_layer, true);
	} else {																				// timer counting down
		// use hour:min so leading 0 is suppressed
		timer_time.tm_hour = -timer_value / 60;
		timer_time.tm_min = -timer_value % 60;
		strftime(timer_buffer, sizeof("00:00"), "%k:%M", &timer_time);
	  text_layer_set_text(s_timer_layer, timer_buffer);	// timer text
		text_layer_set_background_color(s_timer_layer, timer_run ?
			(timer_value < (-1 * 60) ? MyYellow : MyOrange) :
			GColorWhite);
		layer_set_hidden((Layer*)s_count_layer, true);
		layer_set_hidden((Layer*)s_timer_layer, false);
	}
}

// Display timer mode
static void display_mode() {
	switch (timer_mode) {
	case TIMER_STOP:
		text_layer_set_text(s_mode_layer, "DOWN-STOP");
		break;
	case TIMER_ROLLING:
		text_layer_set_text(s_mode_layer, "ROLLING");
		break;
	case TIMER_COUNT:
		text_layer_set_text(s_mode_layer, "DOWN-UP");
		break;
	}
}

static void update_clock_timer() {

	// UPDATE CLOCK
	
	// Get a tm structure
  time_t clock = time(NULL); 
  struct tm *tick_time = localtime(&clock);

  // Create a long-lived buffer
  static char clock_buffer[] = "00:00:00 AM";

  // Write the current time into the buffer
  if(clock_is_24h_style()) {				// 24 hour format
    strftime(clock_buffer, sizeof("00:00:00"), "%H:%M:%S", tick_time);
  } else {													// 12 hour format
    strftime(clock_buffer, sizeof("00:00:00 AM"), "%l:%M:%S %p", tick_time);
  }
  // Display this time on the TextLayer
  text_layer_set_text(s_clock_layer, clock_buffer);

	// USE START TIME IF I HAVE IT, OTHERWISE INCREMENT
	
	if (timer_run) {
		if (start_time) {
			timer_value = clock - start_time;
		} else {
			++timer_value;
		}	
	}
	
	// HANDLE SYNC
	
	if (timer_sync) {
		int delta = ((clock - timer_value) % 60);	// extract secs from start time
		if (delta >= 30) {												// delta to nearest minute
			delta -= 60;
		}
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Sync timer_value:%d delta:%d", (int) timer_value, (int) delta);
		timer_value += delta;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "New timer_value:%d", (int) timer_value);
		timer_sync = false;
		set_start_time(0);					// sync start time
	} 
	
	// UPDATE TIMER
	
  if (timer_run) {
		// VIBES
		switch (timer_value) {			// vibration test
		default:
			if (timer_value != timer_initial + 60) {	// if one minute elapsed from initial_time
				break;
			}																					// then fall through into short vibe
		case -10: case -9: case -8: case -7: case -6: case -5: case -4: case -3: case -2: case -1: 
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Short Vibe");
			vibes_short_pulse();
			break;
		case -1 * 60:
		case 0:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Long Vibe");
			vibes_long_pulse();
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
				set_start_time(start_time - timer_initial);
				break;
			case TIMER_COUNT:					// count down then count UP
				break;
			}
		}
	  display_timer();
  }

	// CAPTURE START TIME
	
	if (timer_run && ! start_time) {
		set_start_time(clock - timer_value);
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
	set_start_time(0);							// sync start time
}

// Up long click, resets timer and inital-value
static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Up long click handled");
	// change timer_initial if timer already stopped with seconds at 0
	if (! timer_run && 0 == timer_value % 60) {
		timer_initial = timer_value;
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "timer_initial now %d minutes", -timer_initial / 60); 
		vibes_double_pulse();
	}
	// reset timer
  timer_run = false;
  timer_sync = false;
	timer_value = timer_initial;
  display_timer();
	set_start_time(0);							// sync start time
}

// Select click toggles start/stop
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Select click handled");
	if ((timer_run = ! timer_run) && timer_mode != TIMER_COUNT && timer_value >= 0) {
		timer_value = timer_initial;
	}
	timer_sync = false;
	display_timer();
	if (timer_run) {
		set_start_time(0);						// sync start time
	}
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
	int correct = (60 - (timer_value % 60)) % 60;
	timer_value += correct ? correct : 60 ;
	// only allow positive time (after start) for TIMER_COUNT
	if (TIMER_COUNT != timer_mode && timer_value > 0) {
		timer_value = 0;
	}
  timer_sync = false;
  display_timer();
	set_start_time(0);							// sync start time
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
	// only allow positive time (after start) for TIMER_COUNT
	if (TIMER_COUNT != timer_mode && timer_value > 0) {	
		timer_value = 0;
		timer_sync = false;
		display_timer();
		set_start_time(0);							// sync start time
	}
	display_mode();
	persist_write_int(PERSIST_TIMER_MODE, (int) timer_mode);
}

// INITIALIZATION

static void click_config_provider(void *context) {
	// Basalt emulator needs much longer time for long click than default
	int long_click_length;
	switch (watch_info_get_model()) {
	case WATCH_INFO_MODEL_UNKNOWN:
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Unknown watch model");
		long_click_length = 2000;
		break;
	default:
		long_click_length = 0;
		break;
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "long_click_length: %d", long_click_length);
	// Platform compatibility
	MyYellow =	COLOR_FALLBACK(GColorYellow,	GColorWhite);
	MyOrange =	COLOR_FALLBACK(GColorOrange,	GColorWhite);
	MyGreen =		COLOR_FALLBACK(GColorGreen,		GColorWhite);
	// Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, long_click_length, up_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, long_click_length, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, long_click_length, down_long_click_handler, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_clock_timer();
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

void show_timer_window(void) {
  initialise_ui();
#ifdef PBL_PLATFORM_APLITE
	window_set_fullscreen(s_window, true);									// full screen on SDK 2
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Set to fullscreen");
#endif
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
	// display mode
	display_mode();
	// retrieve and check start time
	{
	  time_t now = time(NULL); 		// current time
		time_t chk_time = persist_exists(PERSIST_START_TIME) ?
			(int) persist_read_int(PERSIST_START_TIME) :
			0 ;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Retrieved Start time: %d", (int) chk_time);
		if (chk_time && (chk_time > now + 99 * 60 * 60 || chk_time < now - 99 * 60 * 60)) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Retrieved Start time error: %d", (int) chk_time);
			set_start_time(0);
		} else {
			set_start_time(chk_time);
		}
	}
	// run if start time
	if (start_time) {
		timer_run = true;
	} else {
		timer_value = timer_initial;
		display_timer();
	}
	// app running, so cancel any pending wakeups
	wakeup_cancel_all();
}

void hide_timer_window(void) {
  window_stack_remove(s_window, true);
	if (! timer_run) {														// if timer not running
		persist_write_int(PERSIST_START_TIME, 0);		// then clear any start time
	}
}
