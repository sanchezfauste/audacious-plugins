#include "types.h"
#define BUFFER_SIZE 255

LED leds_l[] = {
  {52, -45},
  {50, -40},
  {48, -35},
  {46, -30},
  {44, -25},
  {42, -20},
  {40, -15},
  {38, -10},
  {36, -7},
  {34, -5},
  {32, -3},
  {30, -1},
  {28, 0}
};

int nleds_l = 13;

LED leds_r[] = {
  {53, -45},
  {51, -40},
  {49, -35},
  {47, -30},
  {45, -25},
  {43, -20},
  {41, -15},
  {39, -10},
  {37, -7},
  {35, -5},
  {33, -3},
  {31, -1},
  {29, 0}
};

int nleds_r = 13;

void setup() {
  Serial.begin(115200);
  for (int i=0; i < nleds_l; i += 1) {
    pinMode(leds_l[i].pin, OUTPUT);
  }
  for (int i=0; i < nleds_r; i += 1) {
    pinMode(leds_r[i].pin, OUTPUT);
  }
}

void loop() {
  String serialized_info = Serial.readStringUntil(';');
  ArduinoVUMeterInfo info;
  info.db_l = -96;
  info.db_r = -96;
  info.peak_l = -96;
  info.peak_r = -96;
  char buffer[BUFFER_SIZE];
  int pos = serialized_info.indexOf('$');
  int previous_pos = 0;
  if (pos != -1) {
    serialized_info.substring(previous_pos, pos).toCharArray(buffer, BUFFER_SIZE);
    info.db_l = atof(buffer);
    previous_pos = pos;
    pos = serialized_info.indexOf('$', pos + 1);
    if (pos != -1) {
      serialized_info.substring(previous_pos + 1, pos).toCharArray(buffer, BUFFER_SIZE);
      info.db_r = atof(buffer);
      previous_pos = pos;
      pos = serialized_info.indexOf('$', pos + 1);
      if (pos != -1) {
        serialized_info.substring(previous_pos + 1, pos).toCharArray(buffer, BUFFER_SIZE);
        info.peak_l = atof(buffer);
        previous_pos = pos;
        pos = serialized_info.indexOf('$', pos + 1);
        if (pos != -1) {
          serialized_info.substring(previous_pos + 1, pos).toCharArray(buffer, BUFFER_SIZE);
          info.peak_r = atof(buffer);
        }
      }
    }    
  }

  for (int i = 0; i < nleds_l; i += 1) {
    if (leds_l[i].db <= info.db_l) {
      digitalWrite(leds_l[i].pin, HIGH);
    } else {
      digitalWrite(leds_l[i].pin, LOW);
    }
  }
  for (int i = 0; i < nleds_r; i += 1) {
    if (leds_r[i].db <= info.db_r) {
      digitalWrite(leds_r[i].pin, HIGH);
    } else {
      digitalWrite(leds_r[i].pin, LOW);
    }
  }

  for (int i = nleds_l - 1; i >= 0; i -= 1) {
    if (leds_l[i].db <= info.peak_l) {
      digitalWrite(leds_l[i].pin, HIGH);
      break;
    }
  }
  for (int i = nleds_r - 1; i >= 0; i -= 1) {
    if (leds_r[i].db <= info.peak_r) {
      digitalWrite(leds_r[i].pin, HIGH);
      break;
    }
  }
}


