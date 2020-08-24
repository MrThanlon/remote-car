#pragma once
#ifndef __STAT__
#define __STAT__

#include <Arduino.h>

class Stat {
public:
  // 构造
  Stat();
  // PID参数
  // 绝对角度控制比例系数
  float K_abs = (50 / 45);
  // 角速度控制比例系数
  float K_ang = (50 / 45);
  // 速度控制比例系数
  float K_speed = 1;
  // 速度控制步进比例系数
  float K_step = 1;

  // 上次的心跳包时刻
  unsigned long lastHeartbeat = millis();

  // 上次数据刷新
  unsigned long lastUpdate = millis();

  // 目标绝对角度
  float targetAbsAng = 0;
  // 目标车速
  float targetSpeed = 0;
  //目标角速度
  float targetAng = 0;

  // 目标左轮速度
  float targetLeftSpeed = 0;
  // 目标右轮速度
  float targetRightSpeed = 0;

  // 当前左轮速度
  float currentLeftSpeed = 0;
  //当前右轮速度
  float currentRightSpeed = 0;

  // 角度修正值
  double absAngFix = 0;

  // 当前角度，来自电子罗盘
  float currentAbsAng = 0;

  // 坐标，来自定位
  // 当前x坐标
  float currentX = 0;
  // 当前y坐标
  float currentY = 0;

  // 上次x坐标
  float lastX = 0;
  // 上次y坐标
  float lastY = 0;

  // 当前速度，来自坐标计算
  float currentSpeed = 0;

  // 当前模式，0.绝对角度控制，1.角速度控制，2.手动控制
  int currentMode = 0;

  // 上次计算的时刻，毫秒
  unsigned long lastCalculate = 0;

  // 目标轮速计算
  void calculate();
};

#endif