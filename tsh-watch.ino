#include "hardware.hpp"

Hardware watch;

void setup() {
  // put your setup code here, to run once:
    watch.init();
}

void loop() {
  // put your main code here, to run repeatedly:
    watch.update();
    watch.power_safe_logic();
}
