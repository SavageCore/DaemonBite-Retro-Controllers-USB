//
// NeoGeoControllers32U4 .h
//
// Authors:
//       Jon Thysell <thysell@gmail.com>
//       Mikael Norrgård <mick@daemonbite.com>
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

#ifndef SegaController32U4_h
#define SegaController32U4_h

enum
{
  NG_BTN_UP     = 1,
  NG_BTN_DOWN   = 2,
  NG_BTN_LEFT   = 4,
  NG_BTN_RIGHT  = 8,
  NG_BTN_A      = 16,
  NG_BTN_B      = 32,
  NG_BTN_C      = 64,
  NG_BTN_D      = 128,
  NG_BTN_START  = 256,
  NG_BTN_SELECT = 512,
  NG_BIT_SH_UP = 0
  NG_BIT_SH_DOWN = 1
  NG_BIT_SH_LEFT = 2
  NG_BIT_SH_RIGHT = 3
};

const byte NG_CYCLE_DELAY = 10; // Delay (µs) between setting the select pin and reading the button pins

class NeoGeoControllers32U4  {
  public:
    NeoGeoControllers32U4 (void);
    void readState();
    word currentState[2];

  private:
    void readPort1();
    void readPort2();

    boolean _pinSelect;

    byte _ignoreCycles[2];

    boolean _connected[2];
    boolean _sixButtonMode[2];

    byte _inputReg1;
    byte _inputReg2;
    byte _inputReg3;
};

#endif
