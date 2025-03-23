; clear screen procedure
clscreen proc
    mov ax, 0003h ; alternativa ah,0 al,03 ; sluzba videobios
    int 10h ; prerusenie bios
    ret
endp

; cursor setup (Macro) -> may need to be included ? -> probably not, only if separate file
cursor Macro
    mov dh, x ; kurzor na pozícii (x, y)
    mov dl, y ; grafický režim ms-dos 25 riadkov, 80 stĺpcov
    mov ah, 02h
    int 10h
endm