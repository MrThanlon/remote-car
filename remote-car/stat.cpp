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

    if (currentMode = 0) {
      // 绝对角度控制
      float errorAng = targetAbsAng - (currentAbsAng - absAngFix);
      float errorSpeed = targetSpeed - currentSpeed;
      targetRightSpeed +=
          duration * (K_speed * errorSpeed + K_abs * errorAng / 2);
      targetLeftSpeed +=
          duration * (K_speed * errorSpeed - K_abs * errorAng / 2);

    } else if (currentMode == 1) {
      // 相对角度控制
      float errorSpeed = targetSpeed - currentSpeed;
      targetRightSpeed +=
          duration * (K_speed * errorSpeed + K_abs * targetAng / 2);
      targetLeftSpeed +=
          duration * (K_speed * errorSpeed - K_abs * targetAng / 2);
    }

    // 速度步进
    currentLeftSpeed +=
        duration * K_step * (targetLeftSpeed - currentLeftSpeed);
    currentRightSpeed +=
        duration * K_step * (targetRightSpeed - currentRightSpeed);
  } else {
    // 关闭电机
    currentLeftSpeed = 0;
    currentRightSpeed = 0;
  }
}