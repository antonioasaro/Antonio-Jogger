#include <pebble.h>

static Window *window;
static TextLayer *cntr_layer;
static TextLayer *dist_layer;
static TextLayer *scal_layer;
static TextLayer *time_layer;
static int count = 0;
static int incr  = 0;
static int show = 0;
static int meters = 0;
time_t start;

#define SCALE 1
float scale;

#ifdef PBL_BW
#define YOFF 26
#else
#ifdef PBL_ROUND
#define YOFF 14
#else
#define YOFF 9
#endif
#endif

static float str_to_float(char *str) {
  	float result= 0.0f;
  	size_t dotpos = 0;

  	size_t len = strlen(str);
  	for (size_t n = 0; n < len; n++)  {
    	if (str[n] == '.')    {
      		dotpos = len - n - 1;
    	} else {
      		result = result * 10.0f + (str[n] - '0');
    	}
  	}
  	while ( dotpos--) {
    	result /= 10.0f;
  	}
  	APP_LOG(APP_LOG_LEVEL_INFO, "str_to_float - %s, %d", str, (int) (result * 100));
  	return result;
}

static void update_distance() {
#ifdef PBL_HEALTH	
  	static char dist_text[] = "Dist(km): 0000";  
  	static int offset = 0;
  	time_t end;
	
	APP_LOG(APP_LOG_LEVEL_INFO, "Update distance()");
  	end = time(NULL);
  	HealthMetric metric = HealthMetricWalkedDistanceMeters;
  	HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);
  	if (mask & HealthServiceAccessibilityMaskAvailable) {
		meters = (int) health_service_sum_today(metric);
    	APP_LOG(APP_LOG_LEVEL_DEBUG, "Times: %lld, %lld", (long long) start, (long long) end);
		if (count == 0) { 
	  		offset = meters; 
			text_layer_set_text(dist_layer, "Dist(km): 0.0");
    	} else { 
	  		meters = (meters - offset) * scale;		 		
	    	APP_LOG(APP_LOG_LEVEL_DEBUG, "Meters since start: %d", meters);
      		if (meters > 10000) {
        		dist_text[10] = 48 + (meters / 10000) % 10;
        		dist_text[11] = 48 + (meters / 1000)  % 10;
	    		dist_text[12] = '.';
	    		dist_text[13] = 48 + (meters / 100)   % 10;
	  		} else {
        		dist_text[10] = 48 + (meters / 1000)  % 10;
        		dist_text[11] = '.';
	    		dist_text[12] = 48 + (meters / 100)   % 10;
	    		dist_text[13] =  ' ';
	  		}
	  		text_layer_set_text(dist_layer, dist_text);
		}
  	} else {
    	APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
  	}	
#endif
}

void update_pace() {
#ifdef PBL_HEALTH
	static char pace_text[32] = "00:00 (p)";
	static int prev_meters = 0;
	static int prev_count = 0;
	int curr_meters, delta_meters;
	int curr_count, delta_count;
	int pace;
	
  	APP_LOG(APP_LOG_LEVEL_INFO, "Update pace()");
	curr_meters = meters;
	curr_count = count;
	delta_meters = curr_meters - prev_meters;
	delta_count = curr_count - prev_count;
	APP_LOG(APP_LOG_LEVEL_INFO, "Previous - %d, %d", prev_meters, prev_count);
	APP_LOG(APP_LOG_LEVEL_INFO, "Current  - %d, %d", curr_meters, curr_count);
	if (delta_meters > 1000) {
		if (delta_count > 0) {
			pace = (delta_count * 1000) / ((delta_meters * 1000) / 1000);
			APP_LOG(APP_LOG_LEVEL_INFO, "Current pace - %d", pace);
			pace_text[0] = ' ';
			if (pace > 600) pace_text[0] = 48 + (pace / 600) % 6; 
    		pace_text[1] = 48 + (pace / 60) % 10;
    		pace_text[3] = 48 + (pace / 10) % 6;
    		pace_text[4] = 48 + (pace % 10);
			prev_meters = curr_meters;
			prev_count = curr_count;

			text_layer_set_text_color(scal_layer, GColorBlack);
			text_layer_set_text(scal_layer, pace_text);
		}
	}	
#endif
}

void update_count() {
	static char cntr_text[] = "00:00";  
	
	cntr_text[0] = 48 + (count / 600) % 6;
    cntr_text[1] = 48 + (count / 60) % 10;
    cntr_text[3] = 48 + (count / 10) % 6;
    cntr_text[4] = 48 + (count % 10);
	text_layer_set_text(cntr_layer, cntr_text); 
}

void update_time(struct tm *t) {
	static char hour_text[] = "00:00pm";

	if (clock_is_24h_style()) {
		strftime(hour_text, sizeof(hour_text), "%H:%M", t);
	} else {
		strftime(hour_text, sizeof(hour_text), "%I:%M", t);
	}
	if (hour_text[0] == '0') { hour_text[0] = ' '; }
	if (t->tm_hour < 12) strcat(hour_text, "am"); else strcat(hour_text, "pm");
	text_layer_set_text(time_layer, hour_text);
}

static void update_scale() {
#ifdef PBL_HEALTH
	static char scale_text[32];

	if (persist_exists(SCALE)) {
		char scale_factor[16];
		
    	persist_read_string(SCALE, scale_factor, sizeof(scale_factor));
 		APP_LOG(APP_LOG_LEVEL_INFO, "Read persistent scale_factor - %s", scale_factor);
		scale = str_to_float(scale_factor);
		strcpy(scale_text, "x "); strcat(scale_text, scale_factor);
		text_layer_set_text_color(scal_layer, GColorIndigo);
    	text_layer_set_text(scal_layer, scale_text);
  	} else {
		text_layer_set_text_color(scal_layer, GColorIndigo);
    	text_layer_set_text(scal_layer, "x 1.00");
	}
#endif
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	static int loop = 0;
	
	if (count == 0) loop = 1;
	count = count + incr;
	if (count == ((600*loop)+(60*(loop-1)))) { vibes_long_pulse(); psleep(1000); vibes_long_pulse(); }
	if (count == ((600*loop)+(60*(loop-0)))) { vibes_long_pulse(); loop++; }

	if (incr == 1) {
		if ((count % 30) == 0) { update_distance(); update_pace(); }
		if (((count % 60) == 0) || (show == 1)) update_count(); 
	}
	update_time(tick_time);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  	if (show == 1) show = 0; else show = 1;
	if (incr == 0) { incr = 1; if (count == 0) { start = time(NULL); update_distance(); }}
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	incr = 0;
  	show = 0;
	update_count();
	update_distance();
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  	count = 0;
  	incr = 0;
  	show = 0;
	update_count();
#ifdef PBL_HEALTH
	update_distance();
	update_scale();
#endif
}

void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  	window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  	window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler, select_long_click_release_handler);

}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	
  	APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  	Tuple *scale_factor = dict_find(iterator, SCALE);
  	if (scale_factor) {
    	APP_LOG(APP_LOG_LEVEL_INFO, "Decode & write persist scale_factor - %s", scale_factor->value->cstring);
    	persist_write_string(SCALE, scale_factor->value->cstring);
		update_scale();
  	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void window_load(Window *window) {
  	Layer *window_layer = window_get_root_layer(window);
  	GRect bounds = layer_get_bounds(window_layer);

  	cntr_layer = text_layer_create((GRect) { .origin = { 0, 32+YOFF }, .size = { bounds.size.w, 80 } });
  	text_layer_set_text(cntr_layer, "00:00");
  	text_layer_set_font(cntr_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  	text_layer_set_text_alignment(cntr_layer, GTextAlignmentCenter);
#ifdef PBL_BW	
  	text_layer_set_text_color(cntr_layer, GColorBlack);
#else
  	text_layer_set_text_color(cntr_layer, GColorBlue);
#endif
  	layer_add_child(window_layer, text_layer_get_layer(cntr_layer));

#ifdef PBL_ROUND
  	time_layer = text_layer_create(GRect(42, 14, 100, 28));
#else
  	time_layer = text_layer_create(GRect(28, 8, 100, 28));
#endif
  	text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  	text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  	text_layer_set_text_color(time_layer, GColorDarkGreen);	
  	text_layer_set_background_color(time_layer, GColorWhite);
  	text_layer_set_text(time_layer, "12:34");
  	layer_add_child(window_layer, text_layer_get_layer(time_layer));	
	
#ifdef PBL_HEALTH
  	dist_layer = text_layer_create((GRect) { .origin = { 0, 54+32+YOFF }, .size = { bounds.size.w, 80 } });
  	text_layer_set_text(dist_layer, "Dist(km): 0.0");
  	text_layer_set_font(dist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  	text_layer_set_text_alignment(dist_layer, GTextAlignmentCenter);
  	text_layer_set_text_color(dist_layer, GColorDarkCandyAppleRed);
  	layer_add_child(window_layer, text_layer_get_layer(dist_layer));
	
  	scal_layer = text_layer_create((GRect) { .origin = { 0, 32+56+32+YOFF }, .size = { bounds.size.w, 80 } });
  	text_layer_set_text(scal_layer, "x 1.00");
  	text_layer_set_font(scal_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  	text_layer_set_text_alignment(scal_layer, GTextAlignmentCenter);
  	text_layer_set_text_color(scal_layer, GColorIndigo);
  	layer_add_child(window_layer, text_layer_get_layer(scal_layer));
#endif
}

static void window_unload(Window *window) {
  	text_layer_destroy(cntr_layer);
  	text_layer_destroy(dist_layer);
  	text_layer_destroy(time_layer);
  	text_layer_destroy(scal_layer);
}

static void init(void) {
  	window = window_create();
  	window_set_click_config_provider(window, click_config_provider);
  	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
    	.unload = window_unload,
  	});
  	const bool animated = true;
  	window_stack_push(window, animated);
  	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
	
  	app_message_open(128, 64);
  	app_message_register_inbox_received(inbox_received_callback);
  	app_message_register_inbox_dropped(inbox_dropped_callback);

	scale = 1.00f;
	update_scale();
}

static void deinit(void) {
  	window_destroy(window);
}

int main(void) {
  	init();
  	app_event_loop();
  	deinit();
}