#include <pebble.h>

static Window *window;
static TextLayer *cntr_layer;
static TextLayer *dist_layer;
static TextLayer *time_layer;
static int count = 0;
static int incr  = 0;
static int show = 0;
time_t start;

#ifdef PBL_BW
#define YOFF 24
#else
#ifdef PBL_ROUND
#define YOFF 14
#else
#define YOFF 8
#endif
#endif

static void show_distance() {
#ifdef PBL_HEALTH	
  static char str[] = "Dist(km): 0000";  
  static int offset = 0;
  time_t end;
  int meters;
	
  end = time(NULL);
  HealthMetric metric = HealthMetricWalkedDistanceMeters;
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);
  if (mask & HealthServiceAccessibilityMaskAvailable) {
	meters = (int) health_service_sum_today(metric) - offset;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Times: %lld, %lld", (long long) start, (long long) end);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Meters since start: %d", meters);
	if (count == 0) { offset = meters; return; } 
    if (meters > 10000) {
      str[10] = 48 + (meters / 10000) % 10;
      str[11] = 48 + (meters / 1000)  % 10;
	  str[12] = '.';
	  str[13] = 48 + (meters / 100)   % 10;
	} else {
      str[10] = 48 + (meters / 1000)  % 10;
      str[11] = '.';
	  str[12] = 48 + (meters / 100)   % 10;
	  str[13] =  ' ';
	}
	text_layer_set_text(dist_layer, str);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
  }	
#endif
}

void update_time(struct tm *t) {
static char hourText[] = "00:00pm";

  if(clock_is_24h_style())
	strftime(hourText, sizeof(hourText), "%H:%M", t);
  else
	strftime(hourText, sizeof(hourText), "%I:%M", t);
	if (hourText[0] == '0') { hourText[0] = ' '; }
	if (t->tm_hour < 12) strcat(hourText, "am"); else strcat(hourText, "pm");
	text_layer_set_text(time_layer, hourText);
}


static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (incr == 0) { incr = 1; if (count == 0) { start = time(NULL); show_distance(); }}
  if (show == 1) { show = 0; } else { show = 1; }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  incr = 0;
  show = 0;
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  count = 0;
  incr = 0;
  show = 0;
}

void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler, select_long_click_release_handler);

}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  cntr_layer = text_layer_create((GRect) { .origin = { 0, 32+YOFF }, .size = { bounds.size.w, 80 } });
  text_layer_set_text(cntr_layer, "Jogger");
  text_layer_set_font(cntr_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(cntr_layer, GTextAlignmentCenter);
#ifdef PBL_BW	
  text_layer_set_text_color(cntr_layer, GColorBlack);
#else
  text_layer_set_text_color(cntr_layer, GColorBlue);
#endif
  layer_add_child(window_layer, text_layer_get_layer(cntr_layer));

#ifdef PBL_ROUND
  time_layer = text_layer_create(GRect(42, 14, 100, 20));
#else
  time_layer = text_layer_create(GRect(28, 8, 100, 20));
#endif
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_text_color(time_layer, GColorDarkGreen);	
  text_layer_set_background_color(time_layer, GColorWhite);
  text_layer_set_text(time_layer, "00:00");
  layer_add_child(window_layer, text_layer_get_layer(time_layer));	
	
#ifdef PBL_HEALTH
  dist_layer = text_layer_create((GRect) { .origin = { 0, 56+32+YOFF }, .size = { bounds.size.w, 80 } });
  text_layer_set_text(dist_layer, "Dist(km): 0.0");
  text_layer_set_font(dist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(dist_layer, GTextAlignmentCenter);
  text_layer_set_text_color(dist_layer, GColorDarkCandyAppleRed);
  layer_add_child(window_layer, text_layer_get_layer(dist_layer));
#endif
}

static void window_unload(Window *window) {
  text_layer_destroy(cntr_layer);
  text_layer_destroy(dist_layer);
  text_layer_destroy(time_layer);
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
static char str[] = "00:00";  // 0:00:00";  
static int loop = 0;
	
	if (count == 0) loop = 1;
	count = count + incr;
//	str[0] = 48 + (count / 3600) % 10;
    str[0] = 48 + (count / 600) % 6;
    str[1] = 48 + (count / 60) % 10;
    str[3] = 48 + (count / 10) % 6;
    str[4] = 48 + (count % 10);
	if (count == ((600*loop)+(60*(loop-1)))) { vibes_long_pulse(); psleep(1000); vibes_long_pulse(); }
	if (count == ((600*loop)+(60*(loop-0)))) { vibes_long_pulse(); loop++; }

//	APP_LOG(APP_LOG_LEVEL_DEBUG, "app dbg: %d", count);
	if ((show == 1) || ((count % 60) == 0)) text_layer_set_text(cntr_layer, str);
	if ((incr == 1) && ((count % 30) == 0)) show_distance();
	update_time(tick_time);
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
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}