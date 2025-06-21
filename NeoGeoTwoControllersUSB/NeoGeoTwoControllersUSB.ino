/*  DaemonBite NeoGeo USB Adapter
 *  Author: Mikael Norrgård <mick@daemonbite.com>
 *
 *  Copyright (c) 2020 Mikael Norrgård <http://daemonbite.com>
 *  
 *  GNU GENERAL PUBLIC LICENSE
 *  Version 3, 29 June 2007
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  
 */

#include "NeoGeoControllers32U4.h"
#include "Gamepad.h"

// ATT: 20 chars max (including NULL at the end) according to Arduino source code.
// Additionally serial number is used to differentiate arduino projects to have different button maps!
const char *gp_serial = "NeoGeo to USB";

// Controller DB15 pins (looking face-on to the end of the plug):
//
//    1 2 3 4 5 6 7 8
//  9 10 11 12 13 14 15
//
// Connect pin 8 to +5V and pin 1 to GND
// Connect the remaining pins to digital I/O pins (see below)
// DB15    Arduino Pro Micro
// --------------------------------------
//  15     A0  PF7  // P1 Up
//  7      A1  PF6  // P1 Down
//  14     A2  PF5  // P1 Left
//  6      A3  PF4  // P1 Right
//  13     15  PB3  // P1 B1/A
//  5      14  PB2  // P1 B2/B
//  12     16  PB4  // P1 B2/C
//  4      10  PB6  // P1 B2/D
//  11     9   PB1  // P1 Start
//  3      8   PB0  // P1 Select
//
//  15     3   PD3  // P2 Up
//  7      2   PD2  // P2 Down
//  14     RX  PD0  // P2 Left
//  6      TX  PD1  // P2 Right
//  13     4   PD4  // P2 B1/A
//  5      5   PD5  // P2 B2/B
//  12     6   PD6  // P2 B2/C
//  4      7   PD7  // P2 B2/D
//  11     11  PB7  // P2 Start (reclaimed LED pin)
//  3      13  PB5  // P2 Select (reclaimed LED pin)

// For P2 Start and Select we need to remove the resistors for the onboard LEDs to reclaim 2 GPIO, see here for more info: 
// https://golem.hu/guide/pro-micro-upgrade/
// Start is the left resistor near 8
// Select is the right resistor near 16

NeoGeoControllers32U4 controllers;

// Set up USB HID gamepads
Gamepad_ Gamepad[2];

// Controller previous states
word lastState[2] = {1,1};

void setup()
{
  for(byte gp=0; gp<=1; gp++)
    Gamepad[gp].reset();
}

void loop() { while(1)
{
  controllers.readState();
  sendState(0);
  sendState(1);
}}

void sendState(byte gp)
{
  // Only report controller state if it has changed
  if (controllers.currentState[gp] != lastState[gp])
  {
    Gamepad[gp]._GamepadReport.buttons = controllers.currentState[gp] >> 4;
    Gamepad[gp]._GamepadReport.Y = ((controllers.currentState[gp] & NG_BTN_DOWN) >> NG_BIT_SH_DOWN) - ((controllers.currentState[gp] & NG_BTN_UP) >> NG_BIT_SH_UP);
    Gamepad[gp]._GamepadReport.X = ((controllers.currentState[gp] & NG_BTN_RIGHT) >> NG_BIT_SH_RIGHT) - ((controllers.currentState[gp] & NG_BTN_LEFT) >> NG_BIT_SH_LEFT);
    Gamepad[gp].send();
    lastState[gp] = controllers.currentState[gp];
  }
}
