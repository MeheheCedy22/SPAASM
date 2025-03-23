; Zadanie č. [16]
; ; Autor: Name Surname
; Text zadania:
; Napíšte program (v JSI) ktorý umožní používateľovi pomocou argumentov zadaných na príkazovom riadku pri spúšťaní programu vykonať pre zadaný súbor/súbory (vstup) vybranú funkciu. Ak bude zadaný prepínač '-h', program musí zobraziť informácie o programe a jeho použití. V programe vhodne použite makro s parametrom, ako aj vhodné volania OS (resp. BIOS) pre nastavenie kurzora, výpis reťazca, zmazanie obrazovky, prácu so súbormi a pod. Definície makier musia byť v samostatnom súbore. Program musí korektne spracovať súbory s dĺžkou aspoň do 64kB. Pri čítaní využite pole vhodnej veľkosti (buffer), pričom zo súboru do pamäte sa bude opakovane presúvať vždy (až na posledné čítanie) celá veľkosť poľa. Ošetrite chybové stavy. Program, respektíve každý zdrojový súbor, musí obsahovať primeranú technickú dokumentáciu.
;   - Hlavna uloha:
;       - 16. Vypísať riadky ktoré obsahujú slovo začínajúce veľkým písmenom a ich počet.
;   - Volitelne ulohy:
;       - 7. Plus 1 bod: Ak budú korektne spracované vstupné súbory s veľkosťou nad 64kB.
;       - 12. Plus 1 bod je možné získať za (dobré) komentáre, resp. dokumentáciu, v anglickom jazyku.


; Termín odovzdávania: 23.03.2025 23:59
; Ročník, ak. rok, semester, odbor: tretí, 2024/2025, letný, informatika

; Riešenie:

model small ; small memory model (code and data have each their own single segment)
stack 100h ; stack size 256 B

BUFFSIZE EQU 49152 ; input buffer size 48 KiB

data segment
	help db 'Author: Name Surname', 10, 13, 'Usage: MAIN.EXE [options] [files]', 10, 13, 'Options:', 10, 13, '  -h    Display this help message', 10, 13, 'Files:', 10, 13, '  Specify one file to process.', 10, 13, 'Example: MAIN.EXE INPUT.TXT', 10, 13, '$'

	buff db BUFFSIZE dup ('$'), '$' ; allocate buffer for input data

	buff_line_out db 82 dup ('$'), '$' ; allocate buffer for 1 line of data (80 characters)

	file_name db 16 dup(0), '$'		; buffer for file name (max 16 characters filename)

	number_buffer db 10 dup ('0'), '$' ; Buffer for number conversion (up to 10 digits + terminator)

	file_handle dw ?, '$' ; file handle

	occurance_count dw 0 ; occurance count
	line_start_offset dw 0 ; line start offset (offset in the buffer where the line starts)

	bytes_read dw 0  ; Track how many bytes were actually read

	buffer_start_idx dw 0    ; Where to start processing in current buffer
    incomplete_line db 0     ; Flag: 1 if previous buffer ended with incomplete line
    prev_has_uppercase db 0  ; Tracks if partial line has uppercase word

	newline_str db 13, 10, '$'  ; CR+LF newline sequence for DOS
	occurance_msg db 'Occurance count: ', '$' ; message for occurance count

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

; Include macros
include macros.asm

str_print_service proc
	push ax

	mov ah, 09h		; dos print string service
	int 21h         ; call dos interrupt

	pop ax
	ret
str_print_service endp

; print help
show_help proc
	mov dx, offset help
	call str_print_service 			; call the common string-printing procedure
show_help endp

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
skip_whitespace endp

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

	mov bx, ax           ; save file handle in BX (prerequisite to have the file handle in AX in first place)
	mov ah, 3Fh          ; DOS read file service
	mov cx, BUFFSIZE     ; number of bytes to read
	mov dx, offset buff  ; buffer to receive file data
	int 21h              ; call DOS function (number of bytes read in AX)
	jc read_error        ; if error, jump to handler

	; Store actual bytes read for safe buffer processing
    mov bytes_read, ax   ; Add this line

	jmp read_done

read_error:
	print_str file_read_error_msg
	jmp exit

read_done:
	pop dx
	pop cx
	pop bx
	ret
read_file_to_buff endp

; New procedure to print the contents of the buffer "buff".
print_buff proc
	push dx

	mov dx, offset buff      ; load the offset of buff into DX
	call str_print_service   ; call DOS service via our string-printing procedure

	pop dx
	ret
print_buff endp

print_line proc
	push dx

	mov dx, offset buff_line_out      ; load the offset of buff into DX
	call str_print_service   ; call DOS service via our string-printing procedure

	pop dx
	ret
print_line endp

; CHECK NEWLINES AS \LF ONLY even though DOS uses \CR\LF
process_buff proc
	; push all changed registers to the stack
	push ax
	push bx
	push cx
	push dx

	mov line_start_offset , 0 ; reset line start offset
	mov bx, line_start_offset ; get the current line start offset (current is in bx)

	while_end_of_buff:
		; check for buffer end
	    cmp bx, bytes_read
		; jae process_buff_end
	    jb continue_processing   ; Inverse condition with short jump
		jmp process_buff_end     ; Unconditional jump has no range limitation
		continue_processing:

		; get the current character
		mov al, buff[bx]	

		; if (char is uppercase and offset == 0)
		if_offset_is_zero_and_char_is_uppercase:
			cmp al, 'A'
			jl else_if_char_is_newline
			cmp al, 'Z'
			jg else_if_char_is_newline
			cmp bx, line_start_offset
			je before_finish_line
		
		; if (char is uppercase and [char-1] is whitespace)
		or_if_char_is_uppercase_and_char_minus_one_is_whitespace:
			; check prev char
			mov al, buff[bx - 1]
			cmp al, ' ' ; space 
			je before_finish_line
			cmp al, 9 ; tab
			je before_finish_line
			jmp else_if_char_is_newline

			before_finish_line:
				; counter++
				inc occurance_count
			finish_line_loop:
				inc bx
				cmp bx, bytes_read         ; Check if we've reached the end of valid data
  			 	jae buffer_end_reached     ; If at or beyond end of data, handle specially
				mov al, buff[bx]
				cmp al, 0ah ; check for LF
				je loop_end
				jmp finish_line_loop

			loop_end:
				; print line
				mov si, line_start_offset    ; SI = start position of the current lineart_offset
				mov di, bx                   ; DI = current position (end of line)
				mov cx, di					 ; CX = copy DI
				sub cx, si                   ; CX = length of line (end - start)
				mov dx, offset buff_line_out  ; DX = address of output buffer
				mov bx, cx                    ; BX = save the line length
				mov cx, di					 ; CX = copy DI
				mov di, 0                     ; Reset counter to 0 for the copy loop
				; copy the line to the output buffer
				copy_line_loop:
					mov al, buff[si]          ; Get character from source buffer
					mov buff_line_out[di], al ; Copy to destination buffer
					inc si                    ; Move to next source character
					inc di                    ; Move to next destination position
					cmp di, bx                ; Have we copied the whole line?
					jle copy_line_loop         ; If not, continue copying

				; null terminate the output buffer
				mov buff_line_out[di], '$'
				; do not need, already copying the endline
				; mov buff_line_out[di], 10
				; mov buff_line_out[di+1], 13
				; mov buff_line_out[di+2], '$'

				; print the line
				call print_line

				; set a new start of line
				mov line_start_offset, cx	 ; set the new line start offset (from copied DI earlier)
				inc line_start_offset
				mov bx, line_start_offset ; get the current line start offset (current is in bx)
				jmp while_end_of_buff

		else_if_char_is_newline:
			; increment bx for the next character
			inc bx
			; check for LF
			cmp al, 0ah
			; jne while_end_of_buff
			je is_newline_char       ; Use short jump with opposite condition
			jmp while_end_of_buff    ; Unconditional jump can reach anywhere
			is_newline_char:

			; if (char is newline)
			mov line_start_offset, bx ; set the new line start offset
			jmp while_end_of_buff

	
	buffer_end_reached:
    ; Handle case where line continues beyond current buffer
    ; We'll treat this as a line end for now
    dec bx                     ; Move back to last valid character
    jmp loop_end

	process_buff_end:
		; pop all changed registers from the stack
		pop dx
		pop cx
		pop bx
		pop ax
		ret
process_buff endp

process_file proc
		; call parse_filename to copy file name from command-line
		call parse_filename

		; open file
		open_file file_name
		; store file handle
		mov file_handle, ax

		; read file
	buff_loop:
		mov ax, file_handle
		call read_file_to_buff

		; Check if we read anything
    	cmp bytes_read, 0
    	je end_of_file

		; process file
		call process_buff

		; Continue reading if we filled the buffer (not EOF)
    	cmp bytes_read, BUFFSIZE
    	je buff_loop

	end_of_file:
		; print occurance count
		print_str newline_str
		print_str occurance_msg
		print_number occurance_count


		; close file
		close_file file_handle
		jc file_close_error

		jmp process_file_end

	file_open_error:
		print_str file_open_error_msg
		print_str file_name
		jmp process_file_end

	file_close_error:
		print_str test_str
		print_str file_close_error_msg
		print_str file_name
		jmp process_file_end

	process_file_end:
		ret
process_file endp

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

	args_reverse:
		; handle reverse order argument
		; ...
		print_str test_str_reverse
		jmp args_end

	; file processing without arguments
	args_file:
		; default behavior
		call process_file
		jmp args_end

	args_end:
		ret
handle_args endp

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
; - input file musi mat aspon 2 riadky (obsahovat newline) -- toto asi netreba
; - input file musi mat vzdy za bodku medzeru
; - input file nesmie zacinat s newline ?