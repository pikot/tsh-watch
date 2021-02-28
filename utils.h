#ifndef _UTILS_
#define _UTILS_

#define _SERIAL_DEBUG_ 1

#ifdef _SERIAL_DEBUG_
#define print_w(a) (Serial.print(a))
#define println_w(a) (Serial.println(a))
#define write_w(a) (Serial.write(a))
#define printf_w(...)(Serial.printf(__VA_ARGS__))
#else
#define print_w(a) 
#define println_w(a) 
#define write_w(a) 
#define printf_w(...)
#endif



#endif
