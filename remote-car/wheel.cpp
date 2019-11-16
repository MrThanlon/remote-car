#include "wheel.h"

Wheel::Wheel(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  this->in1 = in1;
  this->in2 = in2;
  this->in3 = in3;
  this->in4 = in4;
}

void Wheel::setSpeed(float left = 0, float right = 0) {
  if (left > 0) {
    analogWrite(in1, left / 100 * PWMRANGE);
    analogWrite(in2, 0);
  } else {
    analogWrite(in2, left / -100 * PWMRANGE);
    analogWrite(in1, 0);
  }

  if (right > 0) {
    analogWrite(in3, right / 100 * PWMRANGE);
    analogWrite(in4, 0);
  } else {
    analogWrite(in4, right / -100 * PWMRANGE);
    analogWrite(in3, 0);
  }
}