#pragma once
#ifndef __CONFIG__
#define __CONFIG__

/**
 * position:
 * B     A
 *
 * C     D
 */
#define PinA 21
#define PinB 22
#define PinC 23
#define PinD 24

#define SOUND_SPEED 0.00034
#define RECV_INTERVAL 3000
// 使用wiringPi中断
#define USE_INTERRUPT 1
// 调试
#define USE_DEBUG 1
// 使用串口接收数据
#define USE_SERIAL 1
// 低通滤波器参数
#define FILTER_PARAM 0.9

#endif