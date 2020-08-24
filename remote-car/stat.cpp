#include "stat.h"

Stat::Stat() {
  lastHeartbeat = 0;
  targetAbsAng = 0;
  targetSpeed = 0;
  targetAng = 0;
  targetLeftSpeed = 0;
  targetRightSpeed = 0;
  currentAbsAng = 0;
  currentMode = 0;
}

void Stat::calculate() {
  // 设定最高计算间隔100ms
  if (millis() - lastCalculate < 100) {
    return;
  }
  // 安全考虑，控制信号3秒内无更新，关闭电机
  if (millis() - lastUpdate < 3000) {
    float duration = (millis() - lastCalculate) / 1e3;
    lastCalculate = millis();
    currentSpeed = sqrt((currentX - lastX) * (currentX - lastX) +
                        (currentY - lastY) * (currentY - lastY)) /
                   duration;

    // FIXME:
    // 对向后的角度做处理(90~270)，另外我觉得并不需要从前端控制速度，而是通过偏转角大小来计算
    if (currentMode == 0) {
      // 绝对角度，PID控制
      float errorAng = targetAbsAng - currentAbsAng;
      float errorSpeed = targetSpeed;
      targetRightSpeed = K_speed * errorSpeed + K_abs * errorAng / 2;
      targetLeftSpeed = K_speed * errorSpeed - K_abs * errorAng / 2;

    } else if (currentMode == 1) {
      // 相对角度控制，直接输出
      float errorSpeed = targetSpeed;
      targetRightSpeed = (K_speed * errorSpeed + K_abs * targetAng / 2);
      targetLeftSpeed = (K_speed * errorSpeed - K_abs * targetAng / 2);
    }
    // 速度限制
    if (currentLeftSpeed > 25) {
      currentLeftSpeed = 25;
    }
    if (currentRightSpeed > 25) {
      currentRightSpeed = 25;
    }

    // 速度步进
    /*
    currentLeftSpeed +=
        duration * K_step * (targetLeftSpeed - currentLeftSpeed);
    currentRightSpeed +=
        duration * K_step * (targetRightSpeed - currentRightSpeed);
        */
    // 没必要比例控制，直接给到就行
    currentLeftSpeed = targetLeftSpeed;
    currentRightSpeed = targetRightSpeed;
  } else {
    // 关闭电机
    currentLeftSpeed = 0;
    currentRightSpeed = 0;
  }
}