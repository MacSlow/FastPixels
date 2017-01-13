////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//
// FastPixels - a experiment into hand-tuned x86-assembly
//
// Copyright 2016-2017 Mirco Müller
//
// Author(s):
//   Mirco "MacSlow" Müller <macslow@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef ASM_H
#define ASM_H

extern "C" bool hasFMA ();
extern "C" bool hasMMX ();
extern "C" bool hasSSE ();
extern "C" bool hasSSE2 ();
extern "C" bool hasSSE3 ();
extern "C" bool hasSSSE3 ();
extern "C" bool hasSSE41 ();
extern "C" bool hasSSE42 ();
extern "C" bool hasAVX ();
extern "C" bool hasAVX2 ();

extern "C" void changeBrightnessASM (const uchar* src, // rdi
                                     uchar* dst,       // rsi
                                     int numBytes,     // rdx
                                     int value);       // rcx

extern "C" void changeBrightnessSSSE3 (const uchar* src, // rdi
                                       uchar* dst,       // rsi
                                       int numBytes,     // rdx
                                       int value);       // rcx

extern "C" void pixelSumHorizAVX (const uchar* src, // rdi
								  uchar* dst,       // rsi
								  int y,            // rdx
								  int width,        // rcx
								  int value);       // r8

extern "C" void pixelSumVertAVX (const uchar* src, // rdi
								 uchar* dst,       // rsi
								 int x,            // rdx
								 int width,        // rcx
								 int height,       // r8
								 int value);       // r9

#endif // ASM_H
