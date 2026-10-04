        LI    r1, 0            ; Zähler
        LI    r2, 1            ; Konstante 1
loop:   ADD   r1, 1, r2        ; Zähler += 1
        STORE r1, 0xF00(r0)    ; Zähler ans LED-Register
        JMP   loop