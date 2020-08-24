/**
 * 从引脚GPIO读取声源位置
 */

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/time.h>
#include <sys/unistd.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#include "config.h"
#include "position.h"

using namespace std;

bool flagInitedValue = true;
uint32_t lastTime = millis();
double lastX = 0;
double lastY = 0;

// 一阶低通滤波
double filter(double raw, double last) {
  if (abs(raw - last) > 0.05 * (millis() - lastTime) / 1000) {
    return last;
  }
  return raw * FILTER_PARAM + last * (1 - FILTER_PARAM);
}

#if USE_SERIAL

char buffer[25];
uint32_t microSeconds[4];

int fd = -1;

void exitFunc() { serialClose(fd); }

int main() {
  wiringPiSetup();
  auto lastUpdate = millis();
  fd = serialOpen("/dev/ttyAMA0", 115200);
  if (fd == -1) {
    cerr << "Failed to open serial, exit" << endl;
    return 1;
  }
  atexit(exitFunc);
  // auto flagSerial = fcntl(fd, F_GETFL);
  // flagSerial &= ~O_NONBLOCK;
  // fcntl(fd, F_SETFL, flagSerial);
  while (1) {
    memset(buffer, '\0', 25);
    int i = 0;
    bool flagFailedToRead = false;
    // 读取到'\n'
    while (1) {
      if (serialDataAvail(fd) > 0) {
        read(fd, buffer + i, 1);
        if (buffer[i] == '\n')
          break;
        if (i > 22) {
          flagFailedToRead = true;
          break;
        }
        i += 1;
      }
      usleep(10);
    }
    if (flagFailedToRead) {
      // puts("Failed to read");
      if (millis() - lastUpdate > 3000) {
        serialClose(fd);
        usleep(1000);
        fd = serialOpen("/dev/ttyAMA0", 115200);
      }
      serialFlush(fd);
      continue;
    }
    double x, y;
    if (sscanf(buffer, "%d%d%d%d", microSeconds, microSeconds + 1,
               microSeconds + 2, microSeconds + 3) != 4) {
      serialFlush(fd);
      continue;
    }
    lastUpdate = millis();
    getPosition(microSeconds[0] * SOUND_SPEED, microSeconds[1] * SOUND_SPEED,
                microSeconds[2] * SOUND_SPEED, microSeconds[3] * SOUND_SPEED, x,
                y);
    // TODO: 高阶滤波
    if (flagInitedValue) {
      flagInitedValue = false;
    } else {
      x = filter(x, lastX);
      y = filter(y, lastY);
    }
    lastX = x;
    lastY = y;
    if (x < 0.5 && y < 0.5 && x > -0.5 && y > -0.5 && !isnan(x) && !isnan(y)) {
      // 有效数据
      printf("%.5f %.5f %u %u %u %u\n", x * 100, y * 100, microSeconds[0],
             microSeconds[1], microSeconds[2], microSeconds[3]);
      fflush(stdout);
    }
  }
  return 0;
}

#else

unsigned int microSecond[4] = {10, 20, 30, 40};
unsigned int microSecond2[4];
// 0.A 1.B 2.C 3.D
int8_t pinFirst = -1;
// 已收集，按位
uint8_t pins;
double x, y;
// 处理中，不接受中断
uint8_t ISRing = 0;
uint8_t ISRready = 0;

bool flagTriged[4] = {false, false, false, false};

// 时间
struct timeval tc;

long int microsGet() {
  gettimeofday(&tc, NULL);
  return tc.tv_usec;
}

void ISR() {
  /*
  printf("raw data: %ld %ld %ld %ld %d %d\n", microSecond[0], microSecond[1],
         microSecond[2], microSecond[2], pinFirst, pins);
  pinFirst = -1;
  pins = 0;
  ISRing = 0;
  ISRready = 0;
  return;
  */
  ISRing = 1;
  microSecond2[0] = microSecond[0];
  microSecond2[1] = microSecond[1];
  microSecond2[2] = microSecond[2];
  microSecond2[3] = microSecond[3];
  auto A = microSecond2[0] - microSecond2[pinFirst];
  auto B = microSecond2[1] - microSecond2[pinFirst];
  auto C = microSecond2[2] - microSecond2[pinFirst];
  auto D = microSecond2[3] - microSecond2[pinFirst];
  if (A > 4118 || B > 4118 || C > 4118 || D > 4118) {
    // 认为无效
    // puts("Error");
    pinFirst = -1;
    pins = 0;
    ISRing = 0;
    ISRready = 0;
    return;
  }
  // count
  getPosition(A * SOUND_SPEED, B * SOUND_SPEED, C * SOUND_SPEED,
              D * SOUND_SPEED, x, y);
  // TODO: 高阶滤波
  if (flagInitedValue) {
    flagInitedValue = false;
  } else {
    x = filter(x, lastX);
    y = filter(y, lastY);
  }
  lastX = x;
  lastY = y;
  if (x < 0.5 && y < 0.5 && x > -0.5 && y > -0.5 && !isnan(x) && !isnan(y)) {
    // 有效数据
    printf("%.5f %.5f %u %u %u %u\n", x * 100, y * 100, A, B, C, D);
  } else {
    /*
    printf("value error,\t%.3f %.3f\t%u %u %u %u %d %u\n", x, y,
           microSecond2[0] - microSecond2[pinFirst],
           microSecond2[1] - microSecond2[pinFirst],
           microSecond2[2] - microSecond2[pinFirst],
           microSecond2[3] - microSecond2[pinFirst], pinFirst, pins);
           */
    // puts("Error");
  }
  fflush(stdout);
  // reset
  pinFirst = -1;
  pins = 0;
  ISRing = 0;
  ISRready = 0;
  flagTriged[0] = false;
  flagTriged[1] = false;
  flagTriged[2] = false;
  flagTriged[3] = false;
}

void ISRpin(uint8_t pin) {
  /*
  if (flagTriged[pin])
    return;
  flagTriged[pin] = true;
  */
  // FIXME: 可能会忽略掉一些
  if ((pins & (1 << pin)) || ISRing || ISRready) {
    // putchar('A' + pin);
    // puts(" repeat");
    return;
  }
  pins |= (1 << pin);
  ISRing = 1;
  unsigned int ms = micros();
  if (ms - microSecond[pin] > RECV_INTERVAL) {
    microSecond[pin] = ms;
    if (pinFirst == -1) {
      // 最快
      pinFirst = pin;
    } /* else {
       // 完成一次收集
       if (pins >= 0xf) {
         ISRready = 1;
       }
     }
     */
  }
  ISRing = 0;
#if USE_DEBUG
  // putchar('A' + pin);
  // puts(" triged");
#endif
}

void ISR_A(void) { ISRpin(0); }

void ISR_B(void) { ISRpin(1); }

void ISR_C(void) { ISRpin(2); }

void ISR_D(void) { ISRpin(3); }

void ioInit() {
  pinMode(PinA, INPUT);
  pinMode(PinB, INPUT);
  pinMode(PinC, INPUT);
  pinMode(PinD, INPUT);
}

void isrInit() {
  wiringPiISR(PinA, INT_EDGE_RISING, ISR_A);
  wiringPiISR(PinB, INT_EDGE_RISING, ISR_B);
  wiringPiISR(PinC, INT_EDGE_RISING, ISR_C);
  wiringPiISR(PinD, INT_EDGE_RISING, ISR_D);
}

int main() {
#if USE_DEBUG
  puts("Position inited.");
#endif
  wiringPiSetup();
  ioInit();
#if USE_INTERRUPT
  isrInit();
#endif

  while (1) {
#if USE_INTERRUPT
    usleep(1000); // 1ms
    if (pins >= 0xf) {
      ISR();
    }
#else
    /**
     * 不使用wiringPi中断
     */

    if ((!(pins & 1)) && digitalRead(PinA)) {
      ISRpin(0);
    }
    if ((!(pins & 2)) && digitalRead(PinB)) {
      ISRpin(1);
    }
    if ((!(pins & 4)) && digitalRead(PinC)) {
      ISRpin(2);
    }
    if ((!(pins & 8)) && digitalRead(PinD)) {
      ISRpin(3);
    }
    if (pins >= 0xf) {
      ISR();
      usleep(RECV_INTERVAL);
    }

#endif
  }
  return 0;
}
#endif