/*
HMC5983.cpp - Class file for the HMC5983 Triple Axis Magnetometer Arduino
Library. Copyright (C) 2011 Love Electronics (loveelectronics.co.uk)/ 2012
bildr.org (Arduino 1.0 compatible)

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Datasheet for HMC5983:
https://www.farnell.com/datasheets/1802211.pdf

*/

#include "HMC5983.h"
#include <Arduino.h>

HMC5983::HMC5983() { m_Scale = 1; }

MagnetometerRaw HMC5983::ReadRawAxis() {
  uint8_t *buffer = Read(DataRegisterBegin, 6);
  MagnetometerRaw raw = MagnetometerRaw();
  raw.XAxis = (buffer[0] << 8) | buffer[1];
  raw.ZAxis = (buffer[2] << 8) | buffer[3];
  raw.YAxis = (buffer[4] << 8) | buffer[5];
  return raw;
}

MagnetometerScaled HMC5983::ReadScaledAxis() {
  MagnetometerRaw raw = ReadRawAxis();
  MagnetometerScaled scaled = MagnetometerScaled();
  scaled.XAxis = raw.XAxis * m_Scale;
  scaled.ZAxis = raw.ZAxis * m_Scale;
  scaled.YAxis = raw.YAxis * m_Scale;
  return scaled;
}

int HMC5983::SetScale(float gauss) {
  Serial.println(gauss);
  Serial.println(gauss == 1.30);
  uint8_t regValue = 0x00;
  if (gauss == 0.88) {
    regValue = 0x00;
    m_Scale = 0.73;
  } else if (gauss == 1.30) {
    regValue = 0x01;
    m_Scale = 0.92;
  } else if (gauss == 1.9) {
    regValue = 0x02;
    m_Scale = 1.22;
  } else if (gauss == 2.5) {
    regValue = 0x03;
    m_Scale = 1.52;
  } else if (gauss == 4.0) {
    regValue = 0x04;
    m_Scale = 2.27;
  } else if (gauss == 4.7) {
    regValue = 0x05;
    m_Scale = 2.56;
  } else if (gauss == 5.6) {
    regValue = 0x06;
    m_Scale = 3.03;
  } else if (gauss == 8.1) {
    regValue = 0x07;
    m_Scale = 4.35;
  } else
    return ErrorCode_1_Num;

  // Setting is in the top 3 bits of the register.
  regValue = regValue << 5;
  Write(ConfigurationRegisterB, regValue);
  return 0;
}

int HMC5983::SetMeasurementMode(uint8_t mode) { Write(ModeRegister, mode); }

void HMC5983::Write(int address, int data) {
  Wire.beginTransmission(HMC5983_Address);
  Wire.write(address);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t *HMC5983::Read(int address, int length) {
  Wire.beginTransmission(HMC5983_Address);
  Wire.write(address);
  Wire.endTransmission();

  Wire.beginTransmission(HMC5983_Address);
  Wire.requestFrom(HMC5983_Address, length);

  uint8_t buffer[length];
  if (Wire.available() == length) {
    for (uint8_t i = 0; i < length; i++) {
      buffer[i] = Wire.read();
    }
  }
  Wire.endTransmission();

  return buffer;
}

char *HMC5983::GetErrorText(int errorCode) {
  if (ErrorCode_1_Num == 1)
    return ErrorCode_1;

  return "Error not defined.";
}