#include <pebble.h>
#include "timer_window.h"

int main() {
  show_timer_window();
  app_event_loop();
  return 0;
}