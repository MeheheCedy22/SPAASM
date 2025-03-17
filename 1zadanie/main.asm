BUFFSIZE EQU 64000 ; input buffer size

DATA SEGMENT
    HELP1 DB 'Author: Marek Cederle','$'
    FILENAME DB 'INPUT.TXT', 0 ; define input file name (max 8 chars)
    BUFF DB BUFFSIZE DUP ('$') ; allocate buffer for input data
    ; ESTR DB ‚ERROR OPENNING FILE$' ; error message if file open fails
DATA ENDS

CODE SEGMENT
; CS -> Code Segment
; DS -> Data Segment
ASSUME CS:CODE, DS:DATA ; init CS, DS (assigning the beginnings of segments to individual segment registers)

; print help
P_HELP:

; clear screen procedure
CLSREEN PROC
    MOV AX, 0003H ; alternativa AH,0 AL,03 ; sluzba videoBIOS
    INT 10H ; prerusenie BIOS
    RET
ENDP

; cursor setup (macro) -> may need to be included ? -> probably not, only if separate file
CURSOR MACRO
    MOV DH, X ; kurzor na pozícii (X, Y)
    MOV DL, Y ; grafický režim MS-DOS 25 riadkov, 80 stĺpcov
    MOV AH, 02H
    INT 10H
ENDM

; write string (macro)
W_STR MACRO placeholder
    MOV DX, OFFSET placeholder ; do DX ulož relatívnu adresu premennej t
    MOV AH, 09H ; niekde v datovom segmente bude: placeholder DB ‚ 'SOME_STRING$'
    INT 21H
ENDM

; open file
; Všetky služby pre prácu so súbormi používajú príznakový bit CF – carry ako výstupný
; parameter, ktorý informuje o tom, či sa služba vykonala bez chyby (CF=0) alebo nie
; (CF=1)
O_FILE MACRO FILENAME
    MOV AH, 3DH ; služba – otvorí súbor podľa spôsobu prístupu v AL
    MOV AL, 0 ; AL 0-read only, 1-write only, 2-read/write
    MOV DX, OFFSET FILENAME ; v dat. segmente- FILENAME DB 'SUBOR.TXT', 0 alebo FILENAME DB 'C:\USERS\TEST.TXT ',0
    INT 21H
ENDM ; pozor!! po skončení je v AX file_handle

; TODO: DAVA ZATIAL ERROR
; close file
; C_FILE:
;     MOV AH, 3EH ; služba MS-DOS
;     MOV BX, FILENAME ; v BX služba MS-DOS 3EH očakáva file_handle
;     INT 21H

; print file
P_FILE MACRO FILENAME ; v dat. segmente je - FILENAME DB 'SUBOR.TXT', 0
    MOV AX, SEG FILENAME ; do AX vlož adresu segmentu DATA
    MOV DS, AX ; presuň do segmentového registra
    MOV DX, OFFSET FILENAME ; do DX ulož relatívnu adresu premennej FILENAME
    MOV AH, 09H ; služba/funkcia MS-DOS
    INT 21H
    ENDM
    ; Plnenie segm. registrov nejde priamo, preto MOV AX, SEG FILENAME
    ; MOV DS, AX

; read file
; R_FILE

START:
    MOV AX,DATA
    MOV DS,AX
    MOV AH,9
    MOV DX, OFFSET HELP1
    INT 21H
    MOV AH,4CH
    INT 21H        
CODE ENDS
END START