#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <unordered_map>

#include "HMC5983.h"
#include "message.h"
#include "wheel.h"

// Wi-Fi SSID
#define WIFI_SSID "pea"

// Wi-Fi密码
#define WIFI_PASS "ka110workhard"

// 服务器地址
#define SERVER_HOST "192.168.6.192"

// 服务器端口
#define SERVER_PORT 9982

// 心跳包间隔
#define HEARTBEAT_INTERVAL 10000

// TCP客户端对象
static WiFiClient client;

// 上次心跳包时刻
unsigned long lastHeartbeat = 0;

/*
const std::unordered_map<const uint8_t, const uint8_t> dataTypeLength = {
    {0x78, 0}, {0x9a, 0}, {0x12, 2}, {0x23, 4},
    {0x34, 4}, {0x56, 4}, {0xbc, 4}};
    */

uint8_t frameBuffer[10];
uint8_t framePointer = 0;
uint8_t frameSum = 0;
uint8_t dataBuffer[5];

// 罗盘
HMC5983 compass;
// 状态
Stat s;
// 消息
Message m;
// 车轮控制
Wheel w(13, 12, 14, 16);

int error = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

  // HMC5983
  Wire.begin();
  compass = HMC5983();
  Serial.println("Constructing new HMC5983");
  Serial.println("Setting scale to +/- 1.3 Ga");
  error = compass.SetScale(1.3); // Set the scale of the compass.
  if (error != 0) {              // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));
  } else {

    Serial.println("Setting measurement mode to continous.");
    // Set the measurement mode to Continuous
    error = compass.SetMeasurementMode(Measurement_Continuous);
    if (error != 0) {
      // If there is an error, print it out.
      Serial.println(compass.GetErrorText(error));
    } else {
      // 初始化固定角度
      MagnetometerRaw raw = compass.ReadRawAxis();
      MagnetometerScaled scaled = compass.ReadScaledAxis();
      float heading = atan2(scaled.YAxis, scaled.XAxis);
      if (heading < 0)
        heading += 2 * PI;

      if (heading > 2 * PI)
        heading -= 2 * PI;

      s.absAngFix = heading * 180 / M_PI;
      Serial.println(s.absAngFix);
    }
  }

  // Wi-Fi
  Serial.print("WiFi connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // TCP client
  Serial.print("TCP Connecting...");
  client.connect(SERVER_HOST, SERVER_PORT);
  while (!client.connected()) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected");

  srand(millis());
}

bool flagSend = false;
char msg[] = "\x99\x12";

// 求平均
int64_t headingAng10Avg = 0;
uint16_t headingAng10Num = 1;

unsigned long lastPrint = millis();

void loop() {
  // TODO: 心跳包读取和重连

  if (!error) {
    // 电子罗盘状态更新
    MagnetometerRaw raw = compass.ReadRawAxis();
    MagnetometerScaled scaled = compass.ReadScaledAxis();
    float heading = atan2(scaled.YAxis, scaled.XAxis);

    // Once you have your heading, you must then add your 'Declination Angle',
    // which is the 'Error' of the magnetic field in your location. Find yours
    // here: http://www.magnetic-declination.com/ Mine is: Longitude: 104° 4' 0"
    // E, which is  -2° 3' , or (which we need) 0.0357792496 radians, I will use
    // 0.0457 If you cannot find your Declination, comment out these two lines,
    // your compass will be slightly off. However, it's not neccessary.
    heading += 0.0357792496;

    if (heading < 0)
      heading += 2 * PI;

    if (heading > 2 * PI)
      heading -= 2 * PI;

    s.currentAbsAng = heading * 180 / M_PI;
    Serial.print("heading: ");
    Serial.println(s.currentAbsAng);
    headingAng10Avg += s.currentAbsAng * 10;
    headingAng10Num += 1;

    if (millis() % 200) {
      flagSend = false;
    } else {
      if (!flagSend) {
        int16_t headingAng10 = (double)headingAng10Avg / headingAng10Num;
        msg[2] = headingAng10 >> 8;
        msg[3] = headingAng10 & 0xff;
        int16_t id = rand();
        msg[4] = id >> 8;
        msg[5] = id & 0xff;
        msg[6] = (0x99 + 0x12 + msg[2] + msg[3] + msg[4] + msg[5]) & 0xff;
        client.write(msg);
        Serial.print("Send heading: ");
        Serial.println(headingAng10);

        headingAng10Avg = 0;
        headingAng10Num = 0;
        flagSend = true;
      }
    }
  }

  // 控制信号
  if (client.available()) {
    // 字节写入
    m.push(client.read());
    //检测完成，执行
    if (m.ready) {
      m.exec(s);
      m.clear();
      s.calculate();
      w.setSpeed(s.currentLeftSpeed, s.currentRightSpeed);
    }
  } else {
    s.calculate();
    w.setSpeed(s.currentLeftSpeed, s.currentRightSpeed);
  }

  if (millis() - lastPrint > 300) {
    Serial.print("speed: ");
    Serial.print(s.currentLeftSpeed);
    Serial.print("|");
    Serial.println(s.currentRightSpeed);
    lastPrint = millis();
  }
}