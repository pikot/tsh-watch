#include "hardware.hpp"

Hardware watch;

void setup() {
      for (int i = 0; i < 0; i++) { 
      int _State = digitalRead(i);

      print_w("status: pin ");
      print_w(i);     print_w(" - "); print_w(_State);     println_w("");  
      }
  // put your setup code here, to run once:
    watch.init();
}

void loop() {
  // put your main code here, to run repeatedly:
    watch.update();
    watch.power_safe_logic();
}
