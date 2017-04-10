			ORG	$0300
ANN0HI		EQU $C059
ANN0LO		EQU $C058
ANN1HI		EQU $C05B
ANN1LO		EQU $C05A
BYTETOSEND	EQU $EF
		
SENDBYTE	LDY   #$08			; load 8 for a full byte, loop counter   
			STA   BYTETOSEND	; put byte in zero page for safe keeping   
RTS			STA   ANN0HI		; annunciator 0 high, RTS   
SENDBIT		ROL   BYTETOSEND	; rotate byte left, high/MSB out to carry (sendbit)   
			BCC   SHORTLOOP		; "branch on carry clear" - if carry/bit = 0, goto #311 (short loop)   
LONGLOOP	LDX   #$14			; if carry/bit = 1, load X with 20 (long loop) 
			JMP   SETANN1		; jump over SHORTLOOP
SHORTLOOP	LDX   #$0A			; if carry/bit = 0, load X with 10 (short loop)   
SETANN1		STA   ANN1HI		; set annunciator 1 HIGH   
COUNTDOWN	DEX					; decrement, countdown to setting ANN1 low (COUNTDOWN)   
			BNE   COUNTDOWN		; if X > 0, keep counting (goto COUNTDOWN)   
			STA   ANN1LO		; if done, set ANN1 LOW, countdown to getting next bit   
			LDX   #$05			; reset counter for short "reset" transition. This lets the GP2IO stage the bit before the next interrupt  
COUNTDOWN2	DEX					; count down again (COUNTDOWN2)   
			BNE   COUNTDOWN2	; if X > 0, keep counting (goto COUNTDOWN2)   
			DEY					; decrement y, countdown bits sent   
			BNE   SENDBIT		; if y > 0, next bit (SENDBIT)   
RTSOFF		STA   ANN0LO		; annunciator 0 low, sending RTS OFF   
			RTS					; return   
		
		