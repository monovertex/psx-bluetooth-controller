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
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"
#include "PS2X_lib.h"
#include "HID_Keycodes.h"
/*********************************************************************************/


// CONFIGURATION & CONSTANTS
/*********************************************************************************/
#define FACTORYRESET_ENABLE 1
#define PSX_DAT             5
#define PSX_CMD             6
#define PSX_ATT             9
#define PSX_CLK             10
#define ANALOG_DEADZONE     25
#define GAMEPAD_READ_DELAY  50
#define SERIAL_RATE         115200
/*********************************************************************************/


// OBJECT INSTANTIATION
/*********************************************************************************/
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
PS2X gamepad;
/*********************************************************************************/


// GLOBALS & HELPERS
/*********************************************************************************/
int buttons[14] = {
  PSB_TRIANGLE, PSB_CIRCLE, PSB_CROSS, PSB_SQUARE,
  PSB_PAD_UP, PSB_PAD_RIGHT, PSB_PAD_DOWN, PSB_PAD_LEFT,
  PSB_L1, PSB_L2,
  PSB_R1, PSB_R2,
  PSB_SELECT, PSB_START
};

int buttonKeyCodes[14] = {
  HID_KEY_Q, HID_KEY_R, HID_KEY_S, HID_KEY_T,
  HID_KEY_ARROW_UP, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_LEFT,
  HID_KEY_A, HID_KEY_S,
  HID_KEY_D, HID_KEY_F,
  HID_KEY_TAB, HID_KEY_RETURN
};

int pressedKeyCodes[14] = { 0 };

unsigned int buttonState = 0;

// Error helper function.
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

String printHex(int input) {
  char s[2];
  sprintf(s, "%02x", input);
  return String(s);
}
/*********************************************************************************/


// SETUP
/*********************************************************************************/
void setup(void) {
  delay(1000);

  Serial.begin(SERIAL_RATE);
  Serial.println(F("=== Adafruit Feather M0 Bluefruit LE Gamepad ==="));
  Serial.println(F("------------------------------------------------"));

  Serial.print(F("--> Initializing the Bluefruit LE module: "));
  if (!ble.begin(VERBOSE_MODE)) { error(F("ERROR: Could not find the module.")); }
  Serial.println(F("OK!"));

  /* Perform a factory reset to make sure everything is in a known state. */
  if (FACTORYRESET_ENABLE) {
    Serial.print(F("--> Performing a factory reset: "));
    if (!ble.factoryReset()){ error(F("ERROR: Could not factory reset.")); }
    Serial.println(F("OK!"));
  }

  /* Disable command echo from Bluefruit. */
  ble.echo(false);

  /* Print Bluefruit information. */
  Serial.println("--> Requesting Bluefruit info:");
  ble.info();

  /* Change the device name to make it easier to find. */
  Serial.print(F("--> Setting device name to 'PlayStation Controller': "));
  if (!ble.sendCommandCheckOK(F("AT+GAPDEVNAME=PlayStation Controller"))) { error(F("ERROR: Could not set device name.")); }
  Serial.println(F("OK!"));

  /* Enable HID Service. */
  Serial.print(F("--> Enabling HID service: "));
  if (!ble.sendCommandCheckOK(F("AT+BLEHIDEN=1"))) { error(F("ERROR: Could not enable HID keyboard service.")); }
  Serial.println(F("OK!"));

  /* Adding or removing requires a reset. */
  Serial.print(F("--> Performing a SW reset: "));
  if (!ble.reset()) { error(F("ERROR: Could not reset.")); }
  Serial.println(F("OK!"));

  Serial.print("--> Initializing & configuring gamepad: ");
  uint8_t gamepadError = gamepad.config_gamepad(PSX_CLK, PSX_CMD, PSX_ATT, PSX_DAT, false, false);
  if (gamepadError) { error(F("ERROR: Could not configure gamepad.")); }
  Serial.println(F("OK!"));

  Serial.println(F("------------------------------------------------"));
  Serial.println(F("====== Gamepad fully configured and ready ======"));
}
/*********************************************************************************/


// LOOP
/*********************************************************************************/
void loop() {
  gamepad.read_gamepad(false, 0);

  unsigned int currentButtonState = gamepad.ButtonDataByte();
  if (buttonState == currentButtonState) { return; }
  buttonState = currentButtonState;

  memset(pressedKeyCodes, 0, sizeof(pressedKeyCodes));
  int pressedIndex = 0;
  int i;
  
  for (i = 0; i < 14; i++) {
    if (!gamepad.Button(buttons[i])) { continue; }
    pressedKeyCodes[pressedIndex] = buttonKeyCodes[i];
    pressedIndex++;
  }

  String command = String("AT+BLEKEYBOARDCODE=00-00");
  for (int i = 0; i < 5; i++) {
    command += String("-") + printHex(pressedKeyCodes[i]);
  }
  
  Serial.print("Sending key code '");
  Serial.print(command);
  Serial.println("': ");
  
  ble.println(command);
  if (ble.waitForOK()) { Serial.println(F("OK!")); }
  else { Serial.println(F("FAILED!")); }
  
  delay(GAMEPAD_READ_DELAY);
}
