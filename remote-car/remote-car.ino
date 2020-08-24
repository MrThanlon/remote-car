#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <unordered_map>

#include "Adafruit_HMC5883_U.h"
#include "HMC5983.h"
#include "message.h"
#include "wheel.h"

// Wi-Fi SSID
#define WIFI_SSID "raspi-webgui"

// Wi-Fi密码
#define WIFI_PASS "ChangeMe"

// 服务器地址
#define SERVER_HOST "10.3.141.1"

// 服务器端口
#define SERVER_PORT 9982

// 心跳包间隔
#define HEARTBEAT_INTERVAL 10000

#define RAD2ANG 57.2957795131

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
uint8_t compassAvailable = 1;

// 罗盘
HMC5983 compass;
// 状态
Stat s;
// 消息
Message m;
// 车轮控制
// Wheel w(D7, D6, D5, D4);
Wheel w(13, 12, 14, 2);

int error = 0;

void setup() {
  // pinMode(LED_BUILTIN, OUTPUT);
  // digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  Serial.println();
  Serial.println("started...");

  // HMC5983
  Wire.begin();
  Wire.beginTransmission(HMC5983_Address);
  if (Wire.endTransmission()) {
    compassAvailable = 0;
    Serial.println("Failed to init campass");
  }
  if (compassAvailable) {
    compass = HMC5983();
    Serial.println("Constructing new HMC5983");
    Serial.println("Setting scale to +/- 1.3 Ga");
    // FIXME: actually, error!=0, 1.3==1.3 return false
    error = compass.SetScale(1.3); // Set the scale of the compass.
    if (error != 0) {              // If there is an error, print it out.
      Serial.println(compass.GetErrorText(error));
    }
    Serial.println("Setting measurement mode to continous.");
    // Set the measurement mode to Continuous
    error = compass.SetMeasurementMode(Measurement_Continuous);
    if (error != 0) {
      // If there is an error, print it out.
      Serial.println(compass.GetErrorText(error));
    }
    // 初始化固定角度
    // 读取10次
    double absAngFix = 0;
    for (int i = 0; i < 10; i++) {
      MagnetometerRaw raw = compass.ReadRawAxis();
      MagnetometerScaled scaled = compass.ReadScaledAxis();
      // FIXME: 忽略无效数据
      double heading = atan2(scaled.YAxis, scaled.XAxis);
      absAngFix += heading;
    }
    s.absAngFix = absAngFix / 10;
    Serial.println(s.absAngFix);
    compassAvailable = 1;
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
  Serial.println("Connected");

  srand(millis());
}

bool flagSend = false;
auto flagLastSendAng = millis();
char msg[7] = "\x99\x12";

// 求平均
int64_t headingAng10Avg = 0;
uint16_t headingAng10Num = 1;

unsigned long lastPrint = millis();
/*
void loop() {
  MagnetometerRaw raw = compass.ReadRawAxis();
  double heading = atan2(raw.YAxis, raw.XAxis);
  if (heading < 0)
    heading += 2 * PI;
  if (heading > 2 * PI)
    heading -= 2 * PI;
  Serial.printf("%f %d %d\n", heading * 180 / M_PI, raw.XAxis, raw.YAxis);
  delay(5);
}
*/
void loop() {
  // TODO: 心跳包读取和重连
  if (millis() - s.lastHeartbeat > 15000) {
    // 重新连接
    Serial.print("TCP Reconnecting...");
    client.connect(SERVER_HOST, SERVER_PORT);
    while (!client.connected()) {
      s.calculate();
      w.setSpeed(s.currentLeftSpeed, s.currentRightSpeed);
    }
    Serial.println("Reconnected");
    s.lastHeartbeat = millis();
  }

  if (compassAvailable) {
    // 电子罗盘状态更新
    MagnetometerRaw raw = compass.ReadRawAxis();
    MagnetometerScaled scaled = compass.ReadScaledAxis();
    /*
    if (millis() - flagLastSendAng > 200) {
      flagLastSendAng = millis();
      Serial.printf("compass raw: %d %d %f %f\n", raw.XAxis, raw.YAxis,
                    scaled.XAxis, scaled.YAxis);
    }
    */
    if (abs(scaled.XAxis) < 1000 && abs(scaled.YAxis) < 1000) {
      // 有效数据
      // Once you have your heading, you must then add your 'Declination Angle',
      // which is the 'Error' of the magnetic field in your location. Find yours
      // here: http://www.magnetic-declination.com/ Mine is: Longitude: 104° 4'
      // 0" E, which is  -2° 3' , or (which we need) 0.0357792496 radians, I
      // will use 0.0457 If you cannot find your Declination, comment out these
      // two lines, your compass will be slightly off. However, it's not
      // neccessary.
      float heading = atan2(scaled.YAxis, scaled.XAxis);
      heading -= s.absAngFix;
      float ang = heading * 180 / M_PI;
      if (ang > 180) {
        ang -= 360;
      }
      s.currentAbsAng = -ang;

      if (millis() - flagLastSendAng > 100) {
        // 间隔200ms
        int16_t headingAng10 = s.currentAbsAng * 10;
        msg[2] = headingAng10 >> 8;
        msg[3] = headingAng10 & 0xff;
        int16_t id = rand();
        msg[4] = id >> 8;
        msg[5] = id & 0xff;
        msg[6] = (0x99 + 0x12 + msg[2] + msg[3] + msg[4] + msg[5]) & 0xff;
        client.write(msg, 7);
        flagLastSendAng = millis();
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

  if (millis() - lastPrint > 100) {
    Serial.print(s.currentLeftSpeed);
    Serial.write('\t');
    Serial.print(s.currentRightSpeed);
    if (compassAvailable) {
      Serial.write('\t');
      Serial.print(s.currentAbsAng);
    }
    Serial.write('\n');
    lastPrint = millis();
  }
}