#include "message.h"

Message::Message() { clear(); }
Message::Message(bool autoExec, Stat &stat) {
  this->autoExec = autoExec;
  this->stat = stat;
  clear();
}

// 插入数据
bool Message::push(uint8_t byte) {
  // 判断帧头
  if (pointer || byte == MESSAGE_HEAD) {
    data[pointer] = byte;
    pointer += 1;
    if (pointer == 2) {
      type = byte;
      switch (type) {
      case 0x78:
        bodyLength = 0;
        break;
      case 0x9a:
      case 0x12:
        bodyLength = 2;
        break;
      case 0x23:
      case 0x34:
      case 0x56:
      case 0xbc:
        bodyLength = 4;
        break;
      default:
        // 不明类型
        clear();
        return false;
      }
    }
    if (pointer >= 5 + bodyLength) {
      // 完成
      // 校验
      if (sum == data[4 + bodyLength]) {
        id = (data[2 + bodyLength] << 8) | data[3 + bodyLength];
        ready = true;
        if (autoExec) {
          exec(stat);
          clear();
        }
      } else {
        Serial.print("Failed to checksum: ");
        Serial.print(sum);
        Serial.print(" ");
        Serial.println(data[4 + bodyLength]);
        clear();
      }
    } else {
      sum += byte;
    }
    return true;
  } else {
    Serial.println("Bad frame");
  }
}

// 清空
void Message::clear() {
  type = 0;
  bodyLength = 0;
  pointer = 0;
  sum = 0;
  ready = false;
}

bool Message::exec(Stat &stat) {
  // 消息未读取完
  if (!ready) {
    return false;
  }
  switch (type) {
  case 0x78: {
    // 更新心跳包时间
    stat.lastHeartbeat = micros();
    break;
  }
  case 0x9a: {
    // 请求重传，目前不需要
    break;
  }
  case 0x12: {
    // 这个是小车发送的，应该不会收到
    return false;
  }
  case 0x23: {
    // 绝对角度和速度控制
    int16_t targetAbsAng = (data[2] << 8) | data[3];
    int16_t targetSpeed = (data[4] << 8) | data[5];
    stat.targetAbsAng = targetAbsAng / 10.0;
    stat.targetSpeed = targetSpeed / 100.0;
    stat.currentMode = 0;
    stat.lastUpdate = millis();
    break;
  }
  case 0x34: {
    // 角速度控制
    int16_t targetAng = (data[2] << 8) | data[3];
    int16_t targetSpeed = (data[4] << 8) | data[5];
    stat.targetAng = targetAng / 10.0;
    stat.targetSpeed = targetSpeed / 100.0;
    stat.currentMode = 1;
    stat.lastUpdate = millis();
    break;
  }
  case 0x56: {
    int16_t targetLeftSpeed = ((int16_t)data[2] << 8) | data[3];
    int16_t targetRightSpeed = ((int16_t)data[4] << 8) | data[5];
    stat.targetLeftSpeed = targetLeftSpeed / 100.0;
    stat.targetRightSpeed = targetRightSpeed / 100.0;
    stat.currentMode = 2;
    stat.lastUpdate = millis();
    break;
  }
  case 0xbc: {
    // 这个应该不会收到
    break;
  }
  default:
    // 不明类型
    return false;
  }
  return true;
}