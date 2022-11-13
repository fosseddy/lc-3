;
; Program to multiply an integer by the constant 6.
; Before execution, an integer must be stored in NUMBER.

        .ORIG x3050
        LEA R2,NUMBER
        LDW R2,R2,#0
        LEA R1,SIX
        LDW R1,R1,#0
        AND R3,R3,#0 ; Clear R3. It will
; contain the product.
; The inner loop
;
AGAIN   ADD R3,R3,R2
        ADD R1,R1,#-1 ; R1 keeps track of
        BRp AGAIN ; the iterations
;
HALT
;
NUMBER  .BLKW 1
SIX     .FILL x0006
;
        .END
