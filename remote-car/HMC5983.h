/*
HMC5983.h - Header file for the HMC5983 Triple Axis Magnetometer Arduino
Library.

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

#pragma once

#ifndef HMC5983_h
#define HMC5983_h

#include <Arduino.h>
#include <Wire.h>

#define HMC5983_Address 0x1E
#define ConfigurationRegisterA 0x00
#define ConfigurationRegisterB 0x01
#define ModeRegister 0x02
#define DataRegisterBegin 0x03

#define Measurement_Continuous 0x00
#define Measurement_SingleShot 0x01
#define Measurement_Idle 0x03

#define ErrorCode_1                                                            \
  "Entered scale was not valid, valid gauss values are: 0.88, 1.3, 1.9, 2.5, " \
  "4.0, 4.7, 5.6, 8.1"
#define ErrorCode_1_Num 1

struct MagnetometerScaled {
  float XAxis;
  float YAxis;
  float ZAxis;
};

struct MagnetometerRaw {
  int XAxis;
  int YAxis;
  int ZAxis;
};

class HMC5983 {
public:
  HMC5983();

  MagnetometerRaw ReadRawAxis();
  MagnetometerScaled ReadScaledAxis();

  int SetMeasurementMode(uint8_t mode);
  int SetScale(float gauss);

  char *GetErrorText(int errorCode);

protected:
  void Write(int address, int byte);
  uint8_t *Read(int address, int length);

private:
  float m_Scale;
};
#endif