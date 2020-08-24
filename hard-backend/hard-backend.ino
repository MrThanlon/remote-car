#include <Arduino.h>

static const uint8_t D5 = 14;
static const uint8_t D6 = 12;
static const uint8_t D7 = 13;
static const uint8_t D8 = 15;

#define PinA 12
#define PinB 13
#define PinC 14
#define PinD 15

#define RECV_INTERVAL 5000

int microSecond[4] = {10, 20, 30, 40};
int microSecond2[4];
// 0.A 1.B 2.C 3.D
int8_t pinFirst = -1;
// 已收集，按位
uint8_t pins;
double x, y;
// 处理中，不接受中断
uint8_t ISRing = 0;
uint8_t ISRready = 0;

uint8_t bitN[] = {1, 2, 4, 8};

ICACHE_RAM_ATTR void ISRpin(uint8_t pin) {
  /*
  if (flagTriged[pin])
    return;
  flagTriged[pin] = true;
  */
  // FIXME: 可能会忽略掉一些
  if (pins & bitN[pin]) {
    // putchar('A' + pin);
    // puts(" repeat");
    return;
  }
  pins |= bitN[pin];
  // ISRing = 1;
  // unsigned int ms = micros();
  // if (ms - microSecond[pin] > 2000) {
  microSecond[pin] = micros();
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
  //}
  // ISRing = 0;
  // putchar('A' + pin);
  // puts(" triged");
}

ICACHE_RAM_ATTR void isr_A() { ISRpin(0); }
ICACHE_RAM_ATTR void isr_B() { ISRpin(1); }
ICACHE_RAM_ATTR void isr_C() { ISRpin(2); }
ICACHE_RAM_ATTR void isr_D() { ISRpin(3); }

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Started.");
  attachInterrupt(PinA, isr_A, RISING);
  attachInterrupt(PinB, isr_B, RISING);
  attachInterrupt(PinC, isr_C, RISING);
  attachInterrupt(PinD, isr_D, RISING);
}

char cache[100];
int A, B, C, D;

ICACHE_RAM_ATTR void loop() {
  delay(2);
  if (pins >= 0xf) {
    noInterrupts();

    A = microSecond[0] - microSecond[pinFirst];
    B = microSecond[1] - microSecond[pinFirst];
    C = microSecond[2] - microSecond[pinFirst];
    D = microSecond[3] - microSecond[pinFirst];

    if (A >= 0 && B >= 0 && C >= 0 && D >= 0 && A < 6000 && B < 6000 &&
        C < 6000 && D < 6000) {
      sprintf(cache, "%d %d %d %d", A, B, C, D);
      Serial.println(cache);
    }
    // reset
    pinFirst = -1;
    pins = 0;
    ISRing = 0;
    ISRready = 0;
    interrupts();
  }
}