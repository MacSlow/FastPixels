;///////////////////////////////////////////////////////////////////////////////
;/3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
;/
;/ FastPixels - a experiment into hand-tuned x86-assembly
;/
;/ Copyright 2016-2017 Mirco Müller
;/
;/ Author(s):
;/   Mirco "MacSlow" Müller <macslow@gmail.com>
;/
;/ This program is free software: you can redistribute it and/or modify it
;/ under the terms of the GNU General Public License version 3, as published
;/ by the Free Software Foundation.
;/
;/ This program is distributed in the hope that it will be useful, but
;/ WITHOUT ANY WARRANTY; without even the implied warranties of
;/ MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
;/ PURPOSE.  See the GNU General Public License for more details.
;/
;/ You should have received a copy of the GNU General Public License along
;/ with this program.  If not, see <http://www.gnu.org/licenses/>.
;/
;///////////////////////////////////////////////////////////////////////////////

;changeBrightnessSSE3 (const uchar* src, ; rdi
;                      uchar* dst,       ; rsi
;                      int numBytes,     ; rdx
;                      int value)        ; rcx
;
; r8, r9, r10, r11 can be freely used without saving
; r9 loop/numBytes-counter
;
; algorithm overview:
; - changeBrightnessSSE does 4 RGBA-pixel in one go
; - value (rcx) is spread across xmm1
; - 4 pixels are read into xmm0 from src
; - xmm0 += xmm1 
; - xmm0 (4 new pixels) is written to dst
; - next 4 pixels
; - and so on until we hit numBytes (rdx)
;
; notes:
; currently if numBytes is not divisable by 16,
; 1..3 pixel at the end are skipped
;
; saturated add/sub still needs to be taken care of
; since currently over- or underflow can happen

section .text
global changeBrightnessSSSE3
changeBrightnessSSSE3:

	xor r9, r9				; r9 is loop-counter, zero it out

	pxor xmm1, xmm1			; zero out xmm1

	pinsrb xmm1, cl, 0		; insert cl to 1st byte of XMM1
	mov rax, 0x8000000080000000 ; set up shuffle selector
	movq xmm0, rax			
	punpcklqdq xmm0, xmm0	
	pshufb xmm1, xmm0		; shuffle xmm1 to have vvv0 for every of the four pixels
	
	cmp cx, 0				; see if we need to add or subtract
	jl subFunc				; jump to subFunc: if cx < 0

loopAdd:
	movdqu xmm0, [rdi+r9]	; get next 4 RGBA-pixels from source
	paddusb xmm0, xmm1		; xmm0 += xmm1 (4 pixels in one go) FIXME: not saturated

	movups [rsi+r9], xmm0	; write 4 pixels to destination, FIXME: right command?

	add r9, 16				; increment offset to next 4 pixels
	cmp r9, rdx				; check if we reached the end
	jb loopAdd				; if not have another go

	ret						; we're done, back to C++

subFunc:
	neg cx					; negate the value

	pinsrb xmm1, cl, 0		; insert cl to 1st byte of XMM1
	mov rax, 0x8000000080000000 ; set up shuffle selector
	movq xmm0, rax			
	punpcklqdq xmm0, xmm0
	pshufb xmm1, xmm0		; shuffle xmm1 to have vvv0 for every of the four pixels

loopSub:
	movdqu xmm0, [rdi+r9]	; get next 4 RGBA-pixels from source
	psubusb xmm0, xmm1		; xmm0 += xmm1 (4 pixels in one go) FIXME: not saturated

	movups [rsi+r9], xmm0	; write 4 pixels to destination, FIXME: right command?

	add r9, 16				; increment offset to next 4 pixels
	cmp r9, rdx				; check if we reached the end
	jb loopSub				; if not have another go

	ret						; we're done, back to C++

