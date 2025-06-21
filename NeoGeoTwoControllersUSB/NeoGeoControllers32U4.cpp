//
// NeoGeoControllers32U4.cpp
//
// Authors:
//       Jon Thysell <thysell@gmail.com>
//       Mikael Norrgård <mick@daemonbite.com>
//
// (Based on the code by Jon Thysell, but the interfacing is almost completely
//  rewritten by Mikael Norrgård)
//
// Copyright (c) 2017 Jon Thysell <http://jonthysell.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Arduino.h"
#include "NeoGeoControllers32U4.h"

#define DEBOUNCE 1          // 1=Diddly-squat-Delay-Debouncing™ activated, 0=Debounce deactivated
#define DEBOUNCE_TIME 10    // Debounce time in milliseconds

bool usbUpdate = false;     // Should gamepad data be sent to USB?
bool debounce = DEBOUNCE;   // Debounce?
uint8_t  pin;               // Used in for loops
uint32_t millisNow = 0;     // Used for Diddly-squat-Delay-Debouncing™

uint8_t  axesDirect = 0x0f;
uint8_t  axes = 0x0f;
uint8_t  axesPrev = 0x0f;
uint8_t  axesBits[4] = {0x10,0x20,0x40,0x80};
uint32_t axesMillis[4];

uint8_t buttonsDirect = 0;
uint8_t buttons = 0;
uint8_t buttonsPrev = 0;
uint8_t buttonsBits[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
uint32_t buttonsMillis[12];

#ifdef DEBUG
  char buf[16];
  uint32_t millisSent = 0;
#endif

NeoGeoControllers32U4::NeoGeoControllers32U4(void)
{
  // Setup input pins for P1 directions (A0,A1,A2,A3) => PF7-PF4 bits 7..4
  DDRF  &= ~B11110000;  // Clear bits 7,6,5,4 to input
  PORTF |=  B11110000;  // Enable pull-up on bits 7,6,5,4

  // Setup button input pins on PORTB (PB0-PB7) for P1 buttons and reclaimed LEDs for P2 start/select
  DDRB  &= ~B11111111;  // Clear bits 7..0 to input
  PORTB |=  B11111111;  // Enable pull-up on bits 7..0

  // Setup input pins for P2 directions and buttons on PORTD (PD0-PD7)
  DDRD  &= ~B11111111;  // Clear bits 7..0 to input
  PORTD |=  B11111111;  // Enable pull-up on bits 7..0

  // Setup output pins (Select lines) on PC6 and PE6
  DDRC  |= B01000000;   // PC6 output
  DDRE  |= B01000000;   // PE6 output
  PORTC |= B01000000;   // PC6 high
  PORTE |= B01000000;   // PE6 high

  _pinSelect = true;
  for(byte i=0; i<=1; i++)
  {
    currentState[i] = 0;
    _connected[i] = 0;
    _sixButtonMode[i] = false;
    _ignoreCycles[i] = 0;
  }

  // Initialize debouncing timestamps
  for(pin=0; pin<4; pin++)
    axesMillis[pin]=0;
  for(pin=0; pin<12; pin++)   
    buttonsMillis[pin]=0;

  #ifdef DEBUG
    Serial.begin(115200);
  #endif  
}


void NeoGeoControllers32U4::readState()
{
  // Set the select pins low/high
  _pinSelect = !_pinSelect;
  if(!_pinSelect) {
    PORTE &= ~B01000000;
    PORTC &= ~B01000000;
  } else {
    PORTE |=  B01000000;
    PORTC |=  B01000000;
  }

  // Short delay to stabilise outputs in controller
  delayMicroseconds(NG_CYCLE_DELAY);

  // Read all input registers
  _inputReg1 = PINF;
  _inputReg2 = PINB;
  _inputReg3 = PIND;

  readPort1();
  readPort2();
}  

void NeoGeoControllers32U4::readPort1()
{
  millisNow = millis();

  for(uint8_t i=0; i<10; i++)
  {
    // Read directions (PF7-PF4)
    axesDirect = ~((PINF & B11110000) >> 4); 
    // Shift down 4 bits to get bits 7..4 into bits 3..0, then invert so pressed=1

    // Map to bits: Up = bit 3 (PF7), Down = bit 2 (PF6), Left = bit 1 (PF5), Right = bit 0 (PF4)
    // We want axesBits = {0x10,0x20,0x40,0x80}, so remap:
    uint8_t axesMapped = 0;
    if(axesDirect & 0x08) axesMapped |= 0x80; // Up (PF7)
    if(axesDirect & 0x04) axesMapped |= 0x40; // Down (PF6)
    if(axesDirect & 0x02) axesMapped |= 0x20; // Left (PF5)
    if(axesDirect & 0x01) axesMapped |= 0x10; // Right (PF4)

    // Read buttons on PORTB: PB0, PB1, PB2, PB3, PB4, PB6
    // We'll read bits 0,1,2,3,4,6 individually and map into a single byte buttonsDirect
    uint8_t pb = PINB;
    buttonsDirect = 0;
    if(!(pb & (1 << 0))) buttonsDirect |= 0x01; // Select (PB0) - bit 0
    if(!(pb & (1 << 1))) buttonsDirect |= 0x02; // Start  (PB1) - bit 1
    if(!(pb & (1 << 2))) buttonsDirect |= 0x04; // B2/B   (PB2) - bit 2
    if(!(pb & (1 << 3))) buttonsDirect |= 0x08; // B1/A   (PB3) - bit 3
    if(!(pb & (1 << 4))) buttonsDirect |= 0x10; // B3/C   (PB4) - bit 4
    if(!(pb & (1 << 6))) buttonsDirect |= 0x20; // B4/D   (PB6) - bit 5 (We only have 6 buttons here)

    if(debounce)
    {
      // Debounce axes
      for(pin=0; pin<4; pin++)
      {
        if((axesMapped & axesBits[pin]) != (axes & axesBits[pin]) && (millisNow - axesMillis[pin]) > DEBOUNCE_TIME)
        {
          axes ^= axesBits[pin];
          axesMillis[pin] = millisNow;
        }
      }

      // Debounce buttons
      // 6 buttons, so loop 0..5
      for(pin=0; pin<6; pin++)
      {
        if((buttonsDirect & buttonsBits[pin]) != (buttons & buttonsBits[pin]) && (millisNow - buttonsMillis[pin]) > DEBOUNCE_TIME)
        {
          buttons ^= buttonsBits[pin];
          buttonsMillis[pin] = millisNow;
        }
      }
    }
    else
    {
      axes = axesMapped;
      buttons = buttonsDirect;
    }

    if(axes != axesPrev)
    {
      // SOCD Cleaner:

      // Extract bits for UP and DOWN
      bool upPressed = (axes & 0x80) != 0;   // bit 7
      bool downPressed = (axes & 0x40) != 0; // bit 6

      // Extract bits for LEFT and RIGHT
      bool leftPressed = (axes & 0x20) != 0; // bit 5
      bool rightPressed = (axes & 0x10) != 0;// bit 4

      // UP + DOWN = NEUTRAL
      int8_t y = 0;
      if(upPressed && !downPressed) y = -1;
      else if(downPressed && !upPressed) y = 1;
      // else y stays 0 if both pressed or neither pressed

      // LEFT + RIGHT = NEUTRAL
      int8_t x = 0;
      if(leftPressed && !rightPressed) x = -1;
      else if(rightPressed && !leftPressed) x = 1;
      // else x stays 0 if both pressed or neither pressed

      Gamepad._GamepadReport.X = x;
      Gamepad._GamepadReport.Y = y;

      axesPrev = axes;
      usbUpdate = true;
    }

    if(buttons != buttonsPrev)
    {
      Gamepad._GamepadReport.buttons = buttons;
      buttonsPrev = buttons;
      usbUpdate = true;
    }

    if(usbUpdate)
    {
      Gamepad.send();
      usbUpdate = false;

      #ifdef DEBUG
        Serial.print("Axes: ");
        Serial.print(Gamepad._GamepadReport.X);
        Serial.print(",");
        Serial.print(Gamepad._GamepadReport.Y);
        Serial.print(" Buttons: ");
        Serial.println(Gamepad._GamepadReport.buttons, BIN);
      #endif
    }
  }
}


void NeoGeoControllers32U4::readPort2()
{
  millisNow = millis();

  for(uint8_t i=0; i<10; i++)
  {
    // Read directions from PORTD pins PD7..PD0 (only relevant bits):
    // PD7 = B2/D (bit 7)
    // PD6 = B3/C (bit 6)
    // PD5 = B2/B (bit 5)
    // PD4 = B1/A (bit 4)
    // PD3 = Up (bit 3)
    // PD2 = Down (bit 2)
    // PD1 = Right (bit 1)
    // PD0 = Left (bit 0)
    //
    // We want to read Up, Down, Left, Right from PD3, PD2, PD0, PD1 respectively,
    // then remap them to bits 7 (Up), 6 (Down), 5 (Left), 4 (Right) as in Port1.

    uint8_t pd = PIND;

    // Invert bits because pressed = LOW on inputs
    uint8_t upPressed = (~pd & (1 << 3)) ? 0x80 : 0x00;     // bit 7
    uint8_t downPressed = (~pd & (1 << 2)) ? 0x40 : 0x00;   // bit 6
    uint8_t leftPressed = (~pd & (1 << 0)) ? 0x20 : 0x00;   // bit 5
    uint8_t rightPressed = (~pd & (1 << 1)) ? 0x10 : 0x00;  // bit 4

    uint8_t axesMapped = upPressed | downPressed | leftPressed | rightPressed;

    // Read buttons:
    // PB7 = Start (bit 1 in buttonsDirect)
    // PB5 = Select (bit 0 in buttonsDirect)
    //
    // Other buttons from PD4..PD7:
    // PD4 = B1/A  -> buttonsDirect bit 3 (0x08)
    // PD5 = B2/B  -> buttonsDirect bit 2 (0x04)
    // PD6 = B3/C  -> buttonsDirect bit 4 (0x10)
    // PD7 = B4/D  -> buttonsDirect bit 5 (0x20)

    uint8_t pb = PINB;

    buttonsDirect = 0;

    if(!(pb & (1 << 5))) buttonsDirect |= 0x01; // Select (PB5) - bit 0
    if(!(pb & (1 << 7))) buttonsDirect |= 0x02; // Start  (PB7) - bit 1

    // Buttons from PIND pins, inverted (pressed = LOW)
    if(~pd & (1 << 5)) buttonsDirect |= 0x04; // B2/B (PD5) - bit 2
    if(~pd & (1 << 4)) buttonsDirect |= 0x08; // B1/A (PD4) - bit 3
    if(~pd & (1 << 6)) buttonsDirect |= 0x10; // B3/C (PD6) - bit 4
    if(~pd & (1 << 7)) buttonsDirect |= 0x20; // B4/D (PD7) - bit 5

    if(debounce)
    {
      // Debounce axes
      for(uint8_t pin=0; pin<4; pin++)
      {
        if((axesMapped & axesBits[pin]) != (axes & axesBits[pin]) && (millisNow - axesMillis[pin]) > DEBOUNCE_TIME)
        {
          axes ^= axesBits[pin];
          axesMillis[pin] = millisNow;
        }
      }

      // Debounce buttons
      for(uint8_t pin=0; pin<6; pin++)
      {
        if((buttonsDirect & buttonsBits[pin]) != (buttons & buttonsBits[pin]) && (millisNow - buttonsMillis[pin]) > DEBOUNCE_TIME)
        {
          buttons ^= buttonsBits[pin];
          buttonsMillis[pin] = millisNow;
        }
      }
    }
    else
    {
      axes = axesMapped;
      buttons = buttonsDirect;
    }

    if(axes != axesPrev)
    {
      // SOCD Cleaner:
      bool up = (axes & 0x80) != 0;
      bool down = (axes & 0x40) != 0;
      bool left = (axes & 0x20) != 0;
      bool right = (axes & 0x10) != 0;

      int8_t y = 0;
      if(up && !down) y = -1;
      else if(down && !up) y = 1;

      int8_t x = 0;
      if(left && !right) x = -1;
      else if(right && !left) x = 1;

      Gamepad._GamepadReport.X = x;
      Gamepad._GamepadReport.Y = y;

      axesPrev = axes;
      usbUpdate = true;
    }

    if(buttons != buttonsPrev)
    {
      Gamepad._GamepadReport.buttons = buttons;
      buttonsPrev = buttons;
      usbUpdate = true;
    }

    if(usbUpdate)
    {
      Gamepad.send();
      usbUpdate = false;

      #ifdef DEBUG
        Serial.print("Axes: ");
        Serial.print(Gamepad._GamepadReport.X);
        Serial.print(",");
        Serial.print(Gamepad._GamepadReport.Y);
        Serial.print(" Buttons: ");
        Serial.println(Gamepad._GamepadReport.buttons, BIN);
      #endif
    }
  }
}


