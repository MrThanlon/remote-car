#pragma once
#ifndef __WHEEL__
#define __WHEEL__

#include <Arduino.h>

// in1,in2控制左轮，in3,in4控制右轮
class Wheel {
public:
  Wheel(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4);
  uint8_t in1, in2, in3, in4;
  // 设左右置轮速度
  void setSpeed(float left, float right);
};

#endif