model small
stack 100h

; 1024
; 2048
; 4096
; 8192
; 16384
; 32768
; 65536

BUFFSIZE EQU 1024 ; input buffer size 16 KiB

data segment
	help db 'Author: Name Surname', 10, 13, 'Usage: MAIN.EXE [options] [files]', 10, 13, 'Options:', 10, 13, '  -h    Display this help message', 10, 13, '  -r    Display contents in reverse order', 10, 13, 'Files:', 10, 13, '  Specify one or more files to process.', 10, 13, 'Example: MAIN.EXE INPUT.TXT', 10, 13, 'Example: MAIN.EXE INPUT.TXT INPUT2.TXT', 10, 13, 'Example: MAIN.EXE -r INPUT.TXT', 10, 13, 'Example: MAIN.EXE -r INPUT.TXT INPUT2.TXT', 10, 13, '$'

	buff db BUFFSIZE dup ('$'), '$' ; allocate buffer for input data

	buff_line_out db 80 dup ('$'), '$' ; allocate buffer for 1 line of data (80 characters)

    file_name db 16 dup(0), '$'		; buffer for file name (max 16 characters filename)

	file_handle dw ?, '$' ; file handle
	
	args_error_msg db 'Invalid arguments. Use -h for help.', '$'
	no_args_error_msg db 'No arguments provided. Use -h for help.', '$'
	file_open_error_msg db 'Error opening file: ', '$'
	file_read_error_msg db 'Error reading from file: ', '$'
	file_close_error_msg db 'Error closing file: ', '$'

	test_str db 'TEST_STRING', '$' ; test string
	test_str_reverse db 'TEST_STR_REVERSE', '$' ; test string 2
data ends

code segment
; cs -> code segment
; ds -> data segment
assume cs:code, ds:data ; init cs, ds (assigning the beginnings of segments to individual segment registers)

; write string (Macro)
print_str Macro str_name
	mov dx, offset str_name     ; do dx ulož relatívnu adresu premennej str_name
	call str_print_service            ; call the common string-printing procedure
endm

; open file
; všetky služby pre prácu so súbormi používajú príznakový bit cf – carry ako výstupný
; parameter, ktorý informuje o tom, či sa služba vykonala bez chyby (cf=0) alebo nie
; (cf=1)
open_file Macro filename
	mov ah, 3dh 				; služba – otvorí súbor podľa spôsobu prístupu v al
	mov al, 0 					; al 0-read only, 1-write only, 2-read/write
	mov dx, offset filename 	; v dat. segmente- filename db 'subor.txt', 0 alebo filename db 'c:\users\test.txt ',0
	int 21h
	jc file_open_error
endm 							; pozor!! po skončení je v ax file_handle

; close file
close_file Macro file_handle
	mov ah, 3eh 			; služba ms-dos
	mov bx, file_handle 	; v bx služba ms-dos 3eh očakáva file_handle
	int 21h
endm

; print file
print_file_name Macro filename 	; v dat. segmente je - filename db 'subor.txt', 0
	mov ax, seg filename 		; do ax vlož adresu segmentu data
	mov ds, ax 					; presuň do segmentového registra
	mov dx, offset filename 	; do dx ulož relatívnu adresu premennej filename
	call str_print_service
endm							; plnenie segm. registrov nejde priamo, preto mov ax, seg filename
								; mov ds, ax

str_print_service proc
	mov ah, 09h		; dos print string service
	int 21h         ; call dos interrupt
	ret
str_print_service endp

; print help
show_help proc
	mov dx, offset help
	call str_print_service 			; call the common string-printing procedure
endp

skip_whitespace proc
	inc si 				; move to to first character
	mov al, es:[si] 	; load the current character into al
	whitespace_loop:
		cmp al, ' ' 	; check if the character is a space
		je skip_char 	; if yes, skip it
		cmp al, 9 		; check if the character is a tab
		je skip_char 	; if yes, skip it
		cmp al, 0dh 	; check if the character is a carriage return
		je skip_char 	; if yes, skip it
		cmp al, 0ah 	; check if the character is a newline
		je skip_char 	; if yes, skip it
		jmp end_skip 	; if no more whitespace, exit the loop

	skip_char:
		inc si 				; move to the next character
		mov al, es:[si] 	; load the next character into al
		jmp whitespace_loop ; repeat the loop

	end_skip:
		ret
endp

; New procedure to parse a file name from command-line arguments.
parse_filename proc
    lea di, file_name       ; point DI to file_name buffer
parse_loop:
    mov al, es:[si]         ; load current character from command-line
    cmp al, ' '             ; stop if space
    je done
    cmp al, 0dh             ; stop if carriage return
    je done
    cmp al, 0ah             ; stop if newline
    je done
    cmp al, 0               ; stop at end of string
    je done
    mov [di], al            ; store character in buffer
    inc di
    inc si
    jmp parse_loop
done:
    mov byte ptr [di], 0    ; null terminate
    ret
parse_filename endp

; New procedure to read the file into the buffer "buff".
read_file_to_buff proc
    ; Assumes the file handle is returned in AX (from open_file).
    push bx
    push cx
    push dx

    mov bx, ax           ; save file handle in BX
    mov ah, 3Fh          ; DOS read file service
    mov cx, BUFFSIZE     ; number of bytes to read
    mov dx, offset buff  ; buffer to receive file data
    int 21h              ; call DOS function (number of bytes read in AX)
    jc read_error        ; if error, jump to handler

    jmp read_done

read_error:
    print_str file_read_error_msg

read_done:
    pop dx
    pop cx
    pop bx
    ret
read_file_to_buff endp

; New procedure to print the contents of the buffer "buff".
print_buff proc
    mov dx, offset buff      ; load the offset of buff into DX
    call str_print_service   ; call DOS service via our string-printing procedure
    ret
print_buff endp

process_buff proc
	; push all changed registers to the stack
	

	process_buff_end:
		; pop all changed registers from the stack
	
		ret
endp

; handle command-line arguments
handle_args proc
	mov ax, 0 					; initialize ax to zero
	mov si, 80h 				; command-line arguments start at 80h in psp
	mov cl, es:[si] 			; get the length of the command-line arguments
	cmp cl, 0
	je no_args 					; if no arguments are present, jump to no_args
	call skip_whitespace
	cmp byte ptr es:[si], '-' 	; check for '-'
	jne args_file 				; if not found, jump to no_args
	inc si 						; move to the next character
	cmp byte ptr es:[si], 'h' 	; check for 'h'
	je args_help
	cmp byte ptr es:[si], 'r' 	; check for 'r'
	je args_reverse
	jmp args_err 				; if no valid arguments are found, jump to args_err

	args_help:
		call show_help 				; show the help message
		jmp args_end 				; exit the program

	args_err:
		print_str args_error_msg 	; show an error message
		jmp args_end 				; exit the program
		
	no_args:
		print_str no_args_error_msg
		jmp args_end

	file_open_error:
		print_str file_open_error_msg
		print_str file_name
		jmp args_end

	file_close_error:
		print_str test_str
		print_str file_close_error_msg
		print_str file_name
		jmp args_end

	args_reverse:
		; handle reverse order argument
		; ...
		print_str test_str_reverse
		jmp args_end

	; file processing without arguments
	args_file:
		; default behavior
    	; call parse_filename to copy file name from command-line
    	call parse_filename
    	; print_file_name file_name   ; display the parsed file name

		; open file
		open_file file_name
		; store file handle
		mov file_handle, ax
		; read file
		call read_file_to_buff
		; print buffer
		call print_buff



		; close file
		close_file file_handle
		jc file_close_error

    	jmp args_end

	args_end:
		ret
endp

; -------------------- START OF THE PROGRAM --------------------
start:
	mov ax,data             ; move data segment address to ax
	mov ds,ax               ; move data segment address to ds
	call handle_args
exit:
	mov ah,4ch ; dos function to terminate the program
	int 21h
code ends
end start





; Zadanie č. [16]
; ; Autor: Name Surname
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




; MOJE PODMIENKY
; - input musi byt max 80 znakov na riadok 
; - input nesmie obsahovat specialny znak '$'
; - input file nazov max 16 znakov