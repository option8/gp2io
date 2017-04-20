			ORG $300

			LDY #$7F
LOOPTY		JSR TWIDDLE
			DEY
			BNE LOOPTY
			RTS		

TWIDDLE     LDA   #$21        
            JSR   PRINTCHAR   
            LDA   #$2F        
            JSR   PRINTCHAR   
            LDA   #$2D        
            JSR   PRINTCHAR   
            LDA   #$1C        
PRINTCHAR   JSR   $FDED     
            LDA   #$AA        
            JSR   $FCA8     
            LDA   #$88        
            JSR   $FDED  
            RTS               
