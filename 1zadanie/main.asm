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

; print help
P_HELP:

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





; Zadanie č. [16]
; Autor: Marek Čederle
; Text zadania:
; Napíšte program (v JSI) ktorý umožní používateľovi pomocou argumentov zadaných na príkazovom riadku pri spúšťaní programu vykonať pre zadaný súbor/súbory (vstup) vybranú funkciu. Ak bude zadaný prepínač '-h', program musí zobraziť informácie o programe a jeho použití. V programe vhodne použite makro s parametrom, ako aj vhodné volania OS (resp. BIOS) pre nastavenie kurzora, výpis reťazca, zmazanie obrazovky, prácu so súbormi a pod. Definície makier musia byť v samostatnom súbore. Program musí korektne spracovať súbory s dĺžkou aspoň do 64kB. Pri čítaní využite pole vhodnej veľkosti (buffer), pričom zo súboru do pamäte sa bude opakovane presúvať vždy (až na posledné čítanie) celá veľkosť poľa. Ošetrite chybové stavy. Program, respektíve každý zdrojový súbor, musí obsahovať primeranú technickú dokumentáciu.
;   - Hlavna uloha:
;       - 16. Vypísať riadky ktoré obsahujú slovo začínajúce veľkým písmenom a ich počet.
;   - Volitelne ulohy:
;       - 7. Plus 1 bod: Ak budú korektne spracované vstupné súbory s veľkosťou nad 64kB.
;       - 8. Plus 1 bod: Prepínač '-r' spôsobí výpis v opačnom poradí (od konca).
;       - 9. Plus 2 body: Ak bude možné zadať viacero vstupných súborov.
;       - 10. Plus 2 body je možné získať ak pridelená úloha bude realizovaná ako externá procedúra (kompilovaná samostatne a prilinkovaná k výslednému programu).
;       - 12. Plus 1 bod je možné získať za (dobré) komentáre, resp. dokumentáciu, v anglickom jazyku.


; Termín odovzdávania: 23.03.2025 23:59
; Ročník, ak. rok, semester, odbor: 3, 2024/2025, letný, informatika


; sem umiestnite okomentovaný program



; Zhodnotenie:
; Uvediete, či je program funkčný - ak nie - prečo?, v akom prostredí bol
; vypracovaný (nutné programové príp. technické vybavenie), chovanie programu
; vzhľadom na vstupné údaje (sú obmedzenia?, mohol by program zlyhať pri
; nesplnení nejakej podmienky?), predpoklady správnej funkčnosti, chovanie
; programu vzhľadom na použité služby DOS resp. BIOS, opis možných vylepšení -
; programové resp. technické, ak sú potrebné, popis použitých algoritmov,
; upozornenie na taktiky a finty, ktoré použil tvorca programu, osobitosti
; riešenia, prípadne použité zdroje.