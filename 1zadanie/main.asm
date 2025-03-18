model small

BUFFSIZE EQU 32768 ; input buffer size

DATA SEGMENT
    HELP DB 'Author: Marek Cederle',10,13, 'Usage: MAIN.EXE [options] [files]',10,13, 'Options:',10,13, '  -h    Display this help message',10,13, '  -r    Display contents in reverse order',10,13, 'Files:',10,13, '  Specify one or more files to process.',10,13, 'Example: MAIN.EXE -r INPUT.TXT INPUT2.TXT',10,13, '$'
    BUFF DB BUFFSIZE DUP ('$') ; allocate buffer for input data
    ; ESTR DB ‚ERROR OPENNING FILE$' ; error message if file open fails
    TEST_STR_NO_ARGS DB 'TEST_STRING_NO_ARGS', '$' ; test string
    TEST_STR_REVERSE DB 'TEST_STR_REVERSE', '$' ; test string 2
    ARGS_ERROR_MSG DB 'Invalid arguments. Use -h for help.', '$'
DATA ENDS

CODE SEGMENT
; CS -> Code Segment
; DS -> Data Segment
ASSUME CS:CODE, DS:DATA ; init CS, DS (assigning the beginnings of segments to individual segment registers)

; clear screen procedure
CLSCREEN PROC
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

; close file
C_FILE MACRO file_handle
    MOV AH, 3EH ; služba MS-DOS
    MOV BX, file_handle ; v BX služba MS-DOS 3EH očakáva file_handle
    INT 21H
ENDM

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
P_HELP MACRO
    MOV DX, OFFSET HELP
    MOV AH, 09H
    INT 21H
ENDM

; handle command-line arguments
HANDLE_ARGS PROC
    MOV AX, 0 ; Initialize AX to zero
    MOV SI, 80H ; Command-line arguments start at 80h in PSP
    MOV CL, ES:[SI] ; Get the length of the command-line arguments
    CMP CL, 0
    JE NO_ARGS ; If no arguments are present, jump to NO_ARGS
    CALL SKIP_WHITESPACE
    CMP BYTE PTR ES:[SI], '-' ; Check for '-'
    JNE NO_ARGS ; If not found, jump to NO_ARGS
    INC SI ; Move to the next character
    CMP BYTE PTR ES:[SI], 'h' ; Check for 'h'
    JE ARGS_HELP
    CMP BYTE PTR ES:[SI], 'r' ; Check for 'r'
    JE ARGS_REVERSE
    JMP ARGS_ERR ; If no valid arguments are found, jump to ARGS_ERR

    ARGS_ERR:
        W_STR ARGS_ERROR_MSG ; Display an error message
        JMP EXIT ; Exit the program

    ARGS_HELP:
        P_HELP ; Display the help message
        JMP EXIT ; Exit the program

    ARGS_REVERSE:
        ; Handle reverse order argument
        ; ...
        W_STR TEST_STR_REVERSE
        JMP ARGS_END

    NO_ARGS: 
        ; Default behavior with files only
        ; ...
        W_STR TEST_STR_NO_ARGS
        JMP ARGS_END

    ARGS_END:
        RET
ENDP

SKIP_WHITESPACE PROC
    INC SI ; Move to to first character
    MOV AL, ES:[SI] ; Load the current character into AL
    WHITESPACE_LOOP:
        CMP AL, ' ' ; Check if the character is a space
        JE SKIP_CHAR ; If yes, skip it
        CMP AL, 9 ; Check if the character is a tab
        JE SKIP_CHAR ; If yes, skip it
        CMP AL, 0DH ; Check if the character is a carriage return
        JE SKIP_CHAR ; If yes, skip it
        CMP AL, 0AH ; Check if the character is a newline
        JE SKIP_CHAR ; If yes, skip it
        JMP END_SKIP ; If no more whitespace, exit the loop

    SKIP_CHAR:
        INC SI ; Move to the next character
        MOV AL, ES:[SI] ; Load the next character into AL
        JMP WHITESPACE_LOOP ; Repeat the loop

    END_SKIP:
        RET
ENDP

START:
    MOV AX,DATA             ; Move data segment address to AX
    MOV DS,AX               ; Move data segment address to DS
    CALL HANDLE_ARGS
EXIT:
    MOV AH,4CH ; DOS function to terminate the program
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