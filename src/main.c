#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *dist_layer;
static int count = 0;
static int incr  = 0;
static int show = 0;
time_t start;

#ifdef PBL_BW
#define YOFF 0
#else
#ifdef PBL_ROUND
#define YOFF 18
#else
#define YOFF 5
#endif
#endif

static void show_distance() {
  // Use the step count metric
  HealthMetric metric = HealthMetricStepCount;
  time_t end = time(NULL);

  // Check the metric has data available for today
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);
  if (mask == HealthServiceAccessibilityMaskAvailable) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Steps today: %d", (int) health_service_sum_today(metric));
	text_layer_set_text(dist_layer, "hi");
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
  }	
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  incr = 1;
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

  text_layer = text_layer_create((GRect) { .origin = { 0, 48+YOFF }, .size = { bounds.size.w, 80 } });
  text_layer_set_text(text_layer, "Jogger");
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
#ifdef PBL_BW	
  text_layer_set_text_color(text_layer, GColorBlack);
#else
  text_layer_set_text_color(text_layer, GColorBlue);
#endif
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
	
  dist_layer = text_layer_create((GRect) { .origin = { 0, 60+48+YOFF }, .size = { bounds.size.w, 80 } });
  text_layer_set_text(dist_layer, "Dist: 0.0 km");
  text_layer_set_font(dist_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(dist_layer, GTextAlignmentCenter);
  text_layer_set_text_color(dist_layer, GColorDarkGreen);
  layer_add_child(window_layer, text_layer_get_layer(dist_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
static char str[] = "00:00";  // 0:00:00";  
static int loop = 0;
	
	if (count == 0) loop = 1;
	count = count + incr;
//	str[0] = 48 + (count / 3600) % 10;
    str[2-2] = 48 + (count / 600) % 6;
    str[3-2] = 48 + (count / 60) % 10;
    str[5-2] = 48 + (count / 10) % 6;
    str[6-2] = 48 + (count % 10);
	if (count == ((600*loop)+(60*(loop-1)))) { vibes_long_pulse(); psleep(1000); vibes_long_pulse(); }
	if (count == ((600*loop)+(60*(loop-0)))) { vibes_long_pulse(); loop++; }

	APP_LOG(APP_LOG_LEVEL_DEBUG, "app dbg: %d", count);
	if ((show == 1) || ((count % 60) == 0)) text_layer_set_text(text_layer, str);
	if ((incr == 1) && ((count % 60) == 0)) show_distance();
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
  start = time(NULL);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}