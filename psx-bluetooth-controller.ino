/**
 *  Adafruit Feather M0 Bluefruit LE Gamepad implementation
 * 
 *  Written by Cosmin Stamate @monovertex, based on the examples at
 *  https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
 * 
 *  This file is an adaptation from https://github.com/adafruit/Adafruit_BluefruitLE_nRF51/blob/master/examples/hidkeyboard/hidkeyboard.ino
 * 
 *  Credits:
 *    - Bill Porter: http://www.billporter.info/2010/06/05/playstation-2-controller-arduino-library-v1-0/
 *    - Evan Kale: https://www.instructables.com/id/Bluetooth-PS2-Controller/
 *    - Ruiz Brothers: https://learn.adafruit.com/custom-wireless-bluetooth-cherry-mx-gamepad/software
 * 
 *  Their work and code helped me understand how to implement this.
 * 
 *  Changelog:
 *    - v0.1.0 - 2019/03/14
 *      Initial implementation, basic button support
 */


// LIBRARIES
/*********************************************************************************/
#include "PS2X_lib.h"
#include <SoftwareSerial.h>
/*********************************************************************************/


// CONFIGURATION & CONSTANTS
/*********************************************************************************/
#define PSX_DAT             11
#define PSX_CMD             10
#define PSX_ATT             5
#define PSX_CLK             4
#define BT_RX               2
#define BT_TX               3
#define AXIS_DEADZONE       40
#define AXIS_RANGE_CENTER   128
#define AXIS_NOISE_LIMIT    2
#define GAMEPAD_READ_DELAY  5
#define SERIAL_RATE         9600
#define BT_SERIAL_RATE      9600
#define INITIAL_DELAY       500
#define DEBUG_TO_SERIAL
/*********************************************************************************/


// OBJECT INSTANTIATION
/*********************************************************************************/
PS2X gamepad;
SoftwareSerial BTSerial(BT_RX, BT_TX);
/*********************************************************************************/


// GLOBALS & HELPERS
/*********************************************************************************/
bool isGamepadInitialized = false;
uint32_t previousButtonState = 0;
int8_t previousX1 = 0;
int8_t previousY1 = 0;
int8_t previousX2 = 0;
int8_t previousY2 = 0;

// Error helper function.
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

int8_t readAxis(int axisIdentifier) { 
  int rawValue = gamepad.Analog(axisIdentifier);
  int normalizedValue = rawValue - AXIS_RANGE_CENTER;
  if (abs(normalizedValue) < AXIS_DEADZONE) { return 0; }
  return (int8_t)normalizedValue;
}

bool areAxisValuesEqual(int8_t valueA, int8_t valueB) {
  return abs(valueA - valueB) <= AXIS_NOISE_LIMIT;
}
/*********************************************************************************/


// SETUP
/*********************************************************************************/
void setup(void) {
  Serial.begin(SERIAL_RATE);
  BTSerial.begin(BT_SERIAL_RATE);
  delay(INITIAL_DELAY);

  uint8_t gamepadError = gamepad.config_gamepad(PSX_CLK, PSX_CMD, PSX_ATT, PSX_DAT, false, false);
  if (gamepadError) { return error(F("ERROR: Could not configure gamepad.")); }
  isGamepadInitialized = true;
}
/*********************************************************************************/


// LOOP
/*********************************************************************************/
void loop() {
  delay(GAMEPAD_READ_DELAY);

  if (!isGamepadInitialized) { return; }

  gamepad.read_gamepad();
  uint32_t currentButtonState = gamepad.ButtonDataByte();
  int8_t currentX1 = readAxis(PSS_LX);
  int8_t currentY1 = readAxis(PSS_LY);
  int8_t currentX2 = readAxis(PSS_RX);
  int8_t currentY2 = readAxis(PSS_RY);

  if (previousButtonState == currentButtonState &&
    areAxisValuesEqual(previousX1, currentX1) &&
    areAxisValuesEqual(previousY1, currentY1) &&
    areAxisValuesEqual(previousX2, currentX2) &&
    areAxisValuesEqual(previousY2, currentY2)) { return; }

  #ifdef DEBUG_TO_SERIAL
    Serial.println(String("Axis values: LX=") + currentX1 + " LY= " + currentY1 + " RX=" + currentX2 + " RY= " + currentY2);
    if (gamepad.Button(PSB_TRIANGLE)) { Serial.println("PSB_TRIANGLE was pressed"); }
    if (gamepad.Button(PSB_CIRCLE)) { Serial.println("PSB_CIRCLE was pressed"); }
    if (gamepad.Button(PSB_CROSS)) { Serial.println("PSB_CROSS was pressed"); } 
    if (gamepad.Button(PSB_SQUARE)) { Serial.println("PSB_SQUARE was pressed"); }
    if (gamepad.Button(PSB_PAD_UP)) { Serial.println("PSB_PAD_UP was pressed"); }
    if (gamepad.Button(PSB_PAD_RIGHT)) { Serial.println("PSB_PAD_RIGHT was pressed"); }
    if (gamepad.Button(PSB_PAD_DOWN)) { Serial.println("PSB_PAD_DOWN was pressed"); }
    if (gamepad.Button(PSB_PAD_LEFT)) { Serial.println("PSB_PAD_LEFT was pressed"); }
    if (gamepad.Button(PSB_L1)) { Serial.println("PSB_L1 was pressed"); }
    if (gamepad.Button(PSB_L2)) { Serial.println("PSB_L2 was pressed"); }
    if (gamepad.Button(PSB_R1)) { Serial.println("PSB_R1 was pressed"); }
    if (gamepad.Button(PSB_R2)) { Serial.println("PSB_R2 was pressed"); }
    if (gamepad.Button(PSB_SELECT)) { Serial.println("PSB_SELECT was pressed"); }
    if (gamepad.Button(PSB_START)) { Serial.println("PSB_START was pressed"); }
  #endif

  previousButtonState = currentButtonState;
  previousX1 = currentX1;
  previousY1 = currentY1;
  previousX2 = currentX2;
  previousY2 = currentY2;

  transmitGamepadStateToBT(currentButtonState, currentX1, currentY1, currentX2, currentY2);
}
/*********************************************************************************/


// TRANSMISSION
/*********************************************************************************/
void transmitGamepadStateToBT(uint32_t buttonState, int8_t x1, int8_t y1, int8_t x2, int8_t y2)
{
  BTSerial.write((uint8_t)0xFD);
  BTSerial.write((uint8_t)0x06);
  BTSerial.write((uint8_t)x1);
  BTSerial.write((uint8_t)y1);
  BTSerial.write((uint8_t)x2);
  BTSerial.write((uint8_t)y2);
  uint8_t buttonState1 = buttonState & 0xFF;
  uint8_t buttonState2 = (buttonState >> 8) & 0xFF;
  BTSerial.write((uint8_t)buttonState1);
  BTSerial.write((uint8_t)buttonState2);
}
