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

;changeBrightnessASM (const uchar* src, ; rdi
;                     uchar* dst,       ; rsi
;                     int numBytes,     ; rdx
;                     int value)        ; rcx
; r8, r9, r10, r11 can be freely used without saving
; r8 loop/numBytes-counter
; r9 tmp-value
;

section .text
global hasFMA
global hasMMX
global hasSSE
global hasSSE2
global hasSSE3
global hasSSSE3
global hasSSE41
global hasSSE42
global hasAVX
global hasAVX2
global changeBrightnessASM

hasFMA:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt ecx, 12
	adc eax, eax
	ret

hasMMX:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt edx, 23
	adc eax, eax
	ret

hasSSE:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt edx, 25
	adc eax, eax
	ret

hasSSE2:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt edx, 26
	adc eax, eax
	ret

hasAVX2:
	mov eax, 0x00000007
	mov ecx, 0
	cpuid
	xor eax, eax
	bt ebx, 5
	adc eax, eax
	ret

hasAVX:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt ecx, 28
	adc eax, eax
	ret

hasSSE3:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt ecx, 0
	adc eax, eax
	ret

hasSSSE3:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt ecx, 9
	adc eax, eax
	ret

hasSSE41:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt ecx, 19
	adc eax, eax
	ret

hasSSE42:
	mov eax, 0x00000001
	cpuid
	xor eax, eax
	bt ecx, 20
	adc eax, eax
	ret

changeBrightnessASM:
	xor r8, r8				; r8 is loop-counter, clear it

	cmp cx, 0				; see if we need to add or subtract
	jl subFunc				; jump to subFunc: if cx < 0

	mov r10w, 0xFFFF		; overflow helper value for add

loopAdd:
	mov al, byte [rdi+r8]	; get next byte from source
	add al, cl				; add value to pixel-component
	cmovc ax, r10w			; clamp al to 255 if overflow

	mov byte [rsi+r8], al	; write new value to destination

	inc r8					; bump loop-counter
	cmp r8, rdx				; check if we reached the end
	jb loopAdd				; if not have another go

	ret						; we're done, back to C++

subFunc:
	mov r10w, 0				; overflow helper value for subtract
	neg cx					; negate the value

loopSub:
	mov al, byte [rdi+r8]	; get next byte from source
	sub al, cl				; subtract value from pixel-component
	cmovc ax, r10w			; clamp al to 0 if underflow

	mov byte [rsi+r8], al	; write new value to destination

	inc r8					; bump loop-counter
	cmp r8, rdx				; check if we reached the end
	jb loopSub				; if not have another go

	ret						; we're done, back to C++

