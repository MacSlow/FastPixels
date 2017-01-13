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

section .data
	align 32
    ones: dd 1.0, 1.0, 1.0, 1.0
    max: dd 0.0, 0.0, 0.0, 0.0
    min: dd 255.0, 255.0, 255.0, 255.0

section .bss
	align 32
	recpAvg: resd 4

section .text
global pixelSumHorizAVX
global pixelSumVertAVX

; - value to check is expected to be pased in xmm4
; and result is returned in eax
; - lower and upper clamp-thresholds are expected
; to be in xmm6 and xmm7 respectively
clampASM:
	minps xmm4, xmm6			; clamp floats in xmm4 to 0.0..255.0
	maxps xmm4, xmm7
	cvttps2dq xmm4, xmm4		; float->int

	packuswb xmm4, xmm4			; "broadcast" and extract 4 bytes...
	packuswb xmm4, xmm4         ; ... into r10d
	pextrd r10d, xmm4, 0

	ret

; TODO/FIXME
;   - verify that reading from src-image 4 uchars are converted to 4 floats
;   - the a-component has to be masked (stay unaltered) out in all operations

;void pixelSumHorizAVX (const uchar* src, // rdi
;					    uchar* dst,       // rsi
;					    int y,            // rdx
;					    int width,        // rcx
;					    int value);       // r8
;
; rax, r10, r11, xmm0-xmm7 can be freely used without saving
; r9:   x
; xmm0: recpAvg
; xmm1: c 
; xmm2: l
; xmm3: lastSum
; xmm4: sum
pixelSumHorizAVX:
	movaps xmm6, [rel min]		; initial preparation for...
	movaps xmm7, [rel max]		; ... min/max-checks in clamp

	cvtsi2ss xmm0, r8			; compute recpArg = 1.f / (2.f * value + 1.f);
	shufps xmm0, xmm0, 0x00		; broadcast lowest single to whole register
	addps xmm0, xmm0
	addps xmm0, [rel ones]
	rcpps xmm0, xmm0
	movaps [rel recpAvg], xmm0

	xor r9, r9					; r9 holds x and starts with 0
	pxor xmm3, xmm3				; zero out lastSum

rowLoop:
	cmp r9, 0					; can we take a shortcut?
	jne slidingWindow

	mov r11, r8					; no shortcut, we've to do the full sum
	neg r11						; r11/offset starts at -value

fullSumLoop:
	mov rax, r9					; check if: x + offset >= 0
	add rax, r11
	cmp rax, 0
	jl advanceFullSumLoop

	mov rax, r9					; check if: x + offset <= width
	add rax, r11
	cmp rax, rcx
	jg advanceFullSumLoop

	mov rax, rdx				; index into src 4 * (y * width + x + offset)
	imul rax, rcx
	add rax, r9
	add rax, r11
	shl rax, 2

	cvtpi2ps xmm5, [rdi + rax]	; sum += src²
	mulps xmm5, xmm5
	addps xmm4, xmm5

advanceFullSumLoop:
	inc r11						; ++offet
	mov r10, r8					; value + 1
	inc r10
	cmp r11, r10				; offset < value + 1
	jl fullSumLoop

	mulps xmm4, xmm0			; sum *= recpAvg

slidingWindow:
	pxor xmm4, xmm4				; zero out sum

	pxor xmm1, xmm1				; by default use 0.0
	mov r10, r9					; if (x + value < width)
	add r10, r8
	cmp r10, rcx
	jg lowerBoundCheck

	mov rax, rdx				; index into src 4 * (y * width + x + value)
	imul rax, rcx
	add rax, r9
	add rax, r8
	shl rax, 2
	cvtpi2ps xmm1, [rdi + rax]

lowerBoundCheck:
	pxor xmm2, xmm2				; by default use 0.0
	mov r10, r9					; if (x - value - 1 >= 0)
	sub r10, r8
	dec r10
	cmp r10, 0
	jl sumUp

	mov rax, rdx				; index into src 4 * (y * width + x - value - 1)
	imul rax, rcx
	add rax, r9
	sub rax, r8
	sub rax, 1
	shl rax, 2
	cvtpi2ps xmm2, [rdi + rax]

sumUp:
	mulps xmm1, xmm1			; sum = lastSum + (c² - l²) * recpAvg
	mulps xmm2, xmm2
	subps xmm1, xmm2
	mulps xmm1, xmm0
	addps xmm3, xmm1
	movaps xmm4, xmm3

	mov rax, rdx				; index into dst, 4 * (y * width + x)
	imul rax, rcx
	add rax, r9
	shl rax, 2

    sqrtps xmm4, xmm4			; dst = clamp (sqrt (sum))
    call clampASM
    mov [rsi + rax], r10d

	movaps xmm3, xmm4			; lastSum = sum

	inc r9						; move to next pixel
	cmp r9, rcx					; reached width yet?
	jl rowLoop

	ret

;void pixelSumVertAVX (const uchar* src, // rdi
;					   uchar* dst,       // rsi
;					   int x,            // rdx
;					   int width,        // rcx
;					   int height,       // r8
;					   int value);       // r9
pixelSumVertAVX:
	ret
