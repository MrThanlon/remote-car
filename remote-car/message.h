#pragma once
#ifndef __MESSAGE__
#define __MESSAGE__

#include "stat.h"
#include <Arduino.h>

#define MESSAGE_HEAD 0x99

// 缓存队列
class Message {
public:
  // 数据
  uint8_t data[10];
  // 指针
  uint8_t pointer = 0;
  // 校验和
  uint8_t sum = 0;
  // 消息类型
  uint8_t type = 0;
  // 帧ID
  uint16_t id = 0;
  // 消息体长度
  uint8_t bodyLength = 0;
  // 自动解析
  bool autoExec = false;
  // 状态
  Stat stat;
  // 构造函数
  Message();
  Message(bool, Stat &);
  // 是否完成读取
  bool ready;
  // 插入数据
  bool push(uint8_t);
  // 清空
  void clear();
  // 执行，分发状态
  bool exec(Stat &);
};

#endif