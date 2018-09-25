#ifndef _TYPES_H_
#define _TYPES_H_

typedef struct ArduinoVUMeterInfo {
    float db_l;
    float db_r;
    float peak_l;
    float peak_r;
} ArduinoVUMeterInfo;

typedef struct LED {
  int pin;
  float db;
} LED;

#endif
