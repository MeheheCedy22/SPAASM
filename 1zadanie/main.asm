; Zadanie č. [16]
; ; Autor: Marek Čederle
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
	; help message string
	help db 'Author: Marek Cederle', 10, 13, 'Usage: MAIN.EXE [options] [files]', 10, 13, 'Options:', 10, 13, '  -h    Display this help message', 10, 13, 'Files:', 10, 13, '  Specify one file to process.', 10, 13, 'Example: MAIN.EXE INPUT.TXT', 10, 13, '$'

	buff db BUFFSIZE dup ('$'), '$' ; buffer for input data
	buff_line_out db 82 dup ('$'), '$' ; allocate buffer for 1 line of data (80 characters with reserve)
	file_name db 16 dup(0), '$'	; buffer for file name (max 16 characters filename, DOS should support only 8)
	number_buffer db 10 dup ('0'), '$' ; Buffer for number conversion (up to 10 digits + terminator)
	file_handle dw ?, '$' ; file handle for opened file

	occurance_count dw 0 		; counter for lines containing words starting with uppercase
	line_start_offset dw 0 		; line start offset (offset in the buffer where the current line starts)
	bytes_read dw 0 			; track how many bytes were actually read from file

	newline_str db 13, 10, '$'  ; CR+LF newline sequence for DOS
	; definitions of error messages
	occurance_msg db 'Occurance count: ', '$'
	args_error_msg db 'Invalid arguments. Use -h for help.', '$'
	no_args_error_msg db 'No arguments provided. Use -h for help.', '$'
	file_open_error_msg db 'Error opening file: ', '$'
	file_read_error_msg db 'Error reading from file: ', '$'
	file_close_error_msg db 'Error closing file: ', '$'
data ends

code segment
; cs -> code segment
; ds -> data segment
assume cs:code, ds:data ; Initialize CS, DS (assigning the beginnings of segments to individual segment registers)

; Include macros from separate file
include macros.asm

; String printing service - Wrapper for DOS function 09h
str_print_service proc
	push ax              ; Save AX register

	mov ah, 09h		     ; DOS function 09h - output string
	int 21h              ; Call DOS interrupt to print string pointed to by DX

	pop ax               ; Restore AX register
	ret
str_print_service endp

; Display help information
show_help proc
	mov dx, offset help  ; Load address of help text
	call str_print_service ; Display the help text
show_help endp

; Skip whitespace characters in command-line input
skip_whitespace proc
	whitespace_loop:
	inc si 					; Move to first character
	mov al, es:[si] 		; Load the current character into AL
		cmp al, ' ' 		; Check if character is a space
		je whitespace_loop 	; If yes, skip it
		cmp al, 9 			; Check if character is a tab (ASCII 9)
		je whitespace_loop 	; If yes, skip it
		cmp al, 0dh 		; Check if character is a carriage return (ASCII 13)
		je whitespace_loop 	; If yes, skip it
		cmp al, 0ah 		; Check if character is a line feed (ASCII 10)
		je whitespace_loop 	; If yes, skip it

		ret
skip_whitespace endp

; Parse filename from command-line arguments
parse_filename proc
	lea di, file_name       ; Point DI to file_name buffer
parse_loop:
	mov al, es:[si]         ; Load current character from command-line
	cmp al, ' '             ; Stop if space
	je done
	cmp al, 0dh             ; Stop if carriage return
	je done
	cmp al, 0ah             ; Stop if line feed
	je done
	cmp al, 0               ; Stop at end of string
	je done
	mov [di], al            ; Store character in buffer
	inc di                  ; Move to next position in destination
	inc si                  ; Move to next position in source
	jmp parse_loop
done:
	mov byte ptr [di], 0    ; Null terminate the filename
	ret
parse_filename endp

; Read file data into the buffer "buff"
read_file_to_buff proc
	; Assumes the file handle is in AX (from open_file)
	push bx                 ; Save registers
	push cx
	push dx

	mov bx, ax              ; Save file handle in BX
	mov ah, 3Fh             ; DOS function 3Fh - read from file
	mov cx, BUFFSIZE        ; Number of bytes to read
	mov dx, offset buff     ; Buffer to receive file data
	int 21h                 ; Call DOS function (bytes read returned in AX)
	jc read_error           ; If error (CF=1), jump to handler

	mov bytes_read, ax      ; Store actual bytes read for safe buffer processing

	jmp read_done

read_error:
	print_str file_read_error_msg  ; Display error message
	jmp exit                       ; Exit program

read_done:
	pop dx                  ; Restore registers
	pop cx
	pop bx
	ret
read_file_to_buff endp

; Print contents of the buffer "buff_line_out"
print_line proc
	push dx                    ; Save DX

	mov dx, offset buff_line_out  ; Load address of line buffer
	call str_print_service        ; Print line buffer contents

	pop dx                     ; Restore DX
	ret
print_line endp

; Process buffer to find lines with words starting with uppercase letters
; - processes newlines as LF only (even though DOS uses CR+LF), CR can be ommited as the LF is the actual last character
process_buff proc
	; Save registers that will be modified
	push ax
	push bx
	push cx
	push dx

	mov line_start_offset, 0   ; Reset line start offset
	mov bx, line_start_offset  ; BX = current position in buffer

	while_end_of_buff:
		; Check if we've reached the end of buffer
		cmp bx, bytes_read
		jb continue_processing   ; Inverse condition with short jump
		jmp process_buff_end     ; Unconditional jump has no range limitation (workaround because i reached the jmp limit)
		continue_processing:

		; Get the current character
		mov al, buff[bx]	

		; Check if current char is uppercase and if the current character is first character of the line
		if_offset_is_zero_and_char_is_uppercase:
			cmp al, 'A'               ; Is it less than 'A'?
			jl else_if_char_is_newline
			cmp al, 'Z'               ; Is it greater than 'Z'?
			jg else_if_char_is_newline
			cmp bx, line_start_offset  ; Is it first character of line?
			je before_finish_line      ; If uppercase at line start, count the line (occurance)
		
		; Check if current char is uppercase and whitespace precedes it
		or_if_char_is_uppercase_and_char_minus_one_is_whitespace:
			; Check previous character
			mov al, buff[bx - 1]
			cmp al, ' '              ; Is previous char a space?
			je before_finish_line
			cmp al, 9                ; Is previous char a tab?
			je before_finish_line
			jmp else_if_char_is_newline

			; Found uppercase word - increment counter
			before_finish_line:
				inc occurance_count
			; Scan to end of line (next LF)
			finish_line_loop:
				inc bx					   ; Increment counter for next character
				cmp bx, bytes_read         ; Check if we've reached buffer end
			   	jae buffer_end_reached     ; If so, handle end of buffer
				mov al, buff[bx]		   ; Get the current character
				cmp al, 0ah                ; Check for LF (line feed)
				je loop_end                ; If found, process line
				jmp finish_line_loop       ; Otherwise continue scanning

			; Process a matching line (contains uppercase word) and print it
			loop_end:
				mov si, line_start_offset    ; SI = start of current line
				mov di, bx                   ; DI = end of line (position of LF)
				mov cx, di                   ; Save end position
				sub cx, si                   ; CX = length of line
				mov dx, offset buff_line_out ; DX = output buffer address
				mov bx, cx                   ; BX = line length
				mov cx, di                   ; Restore end position
				mov di, 0                    ; DI = output position counter
				; Copy the line to output buffer
				copy_line_loop:
					mov al, buff[si]          ; Get character from source
					mov buff_line_out[di], al ; Copy to output buffer
					inc si                    ; Next source character
					inc di                    ; Next output position
					cmp di, bx                ; Reached end of line?
					jle copy_line_loop        ; If not, continue copying

				mov buff_line_out[di], '$'   ; Add string terminator for DOS output

				call print_line ; Print the matched line

				; Update to start of next line
				mov line_start_offset, cx    ; Update line start to end of current + 1
				inc line_start_offset
				mov bx, line_start_offset    ; BX = new (current) position
				jmp while_end_of_buff        ; Continue with next line

		; check if character is newline, update the line start offset and continue the loop
		else_if_char_is_newline:
			inc bx                     ; Move to next character
			cmp al, 0ah                ; Is it a line feed?
			je is_newline_char         ; Inverse condition with short jump
			jmp while_end_of_buff      ; Unconditional jump has no range limitation (workaround because i reached the jmp limit)
			is_newline_char:

			; Found a newline - update line start to next character
			mov line_start_offset, bx  ; Start of next line
			jmp while_end_of_buff      ; Continue processing
	
	; Handle case where a line continues beyond buffer end treat current position as line end
	buffer_end_reached:
		dec bx                     ; Move back to last valid character
		jmp loop_end               ; Process as end of line

	process_buff_end:
		; Restore registers
		pop dx
		pop cx
		pop bx
		pop ax
		ret
process_buff endp

; Process an input file to find lines with uppercase words
process_file proc
		
	call parse_filename ; Parse the filename from command line

	open_file file_name ; Open the input file
	mov file_handle, ax ; Store file handle for later use

	; Read and process file in chunks until EOF
	buff_loop:
		mov ax, file_handle
		call read_file_to_buff      ; Read chunk into buffer

		cmp bytes_read, 0 			; Check if any bytes were read
		je end_of_file              ; If zero bytes read, at end of file

		call process_buff 			; Process current buffer contents

		; Continue reading if buffer was filled (more data available)
		cmp bytes_read, BUFFSIZE
		je buff_loop                ; If buffer full, read next chunk

	; Print final count of matching lines
	end_of_file:
		print_str newline_str        ; Print newline as a separator
		print_str occurance_msg      ; Print occurance message
		print_number occurance_count ; Print the actual count value

		close_file file_handle		; Close the file
		jc file_close_error         ; Handle close error if it occurs

		jmp process_file_end

	; Handle file open error (print error message)
	file_open_error:
		print_str file_open_error_msg
		print_str file_name
		jmp process_file_end

	; Handle file close error (print error message)
	file_close_error:
		print_str file_close_error_msg
		print_str file_name
		jmp process_file_end

	process_file_end:
		ret
process_file endp

; Handle command-line arguments and determine program operation
handle_args proc
	mov ax, 0 					; Initialize AX to zero
	mov si, 80h 				; Get start of PSP {Command-line arguments start at 80h in PSP (Program Segment Prefix)}
	mov cl, es:[si] 			; Get length of command-line arguments
	cmp cl, 0                   ; Check if any arguments provided
	je no_args 					; If no arguments, show usage info
	call skip_whitespace        ; Skip any leading whitespace
	cmp byte ptr es:[si], '-' 	; Check for option flag (-)
	jne args_file 				; If not option, treat as filename
	inc si 						; Move to character after '-'
	cmp byte ptr es:[si], 'h' 	; Check for help option (-h)
	je args_help
	jmp args_err 				; If not valid option, show error

	args_help:
		call show_help 				; Display help information
		jmp args_end 				; End processing

	args_err:
		print_str args_error_msg 	; Show invalid argument error
		jmp args_end 				; End processing
		
	no_args:
		print_str no_args_error_msg ; Show no arguments error
		jmp args_end                ; End processing

	; Process input file (default operation)
	args_file:
		call process_file           ; Process the file specified in arguments
		jmp args_end

	args_end:
		ret
handle_args endp

; -------------------- START OF THE PROGRAM --------------------
start:
	mov ax,data             ; Initialize data segment
	mov ds,ax               ; DS = address of data segment
	call handle_args        ; Process command line arguments and determine operation
exit:
	mov ah,4ch              ; DOS function 4Ch - terminate program
	int 21h                 ; Call DOS to exit
code ends
end start
; -------------------- END OF THE PROGRAM --------------------


; Zhodnotenie:

; Program je funkčný, spracováva zadaný vstupný súbor a vypisuje riadky, ktoré
; obsahujú slovo začínajúce veľkým písmenom. Program bol vypracovaný v prostredí
; DOSBox 0.74 s použitím assemblera TASM (Turbo assembler) 2.02. Program bol 
; testovaný na súboroch s rôznymi dĺžkami a obsahom, vrátane súborov väčších ako
; 64kB. Program má implementovanú kontrolu chýb pri otváraní a čítaní súborov, ako aj
; pri spracovaní vstupných argumentov. V prípade nesplnenia podmienok sa zobrazí
; chybová hláška a program sa ukončí. Program má aj help funkciu, ktorá zobrazuje
; informácie o použití programu a jeho možnostiach. Program obsahuje aj ďalší súbor
; s definíciami makier, ktoré sú použité v hlavnom programe. Nizšie sú uvedené podmienky,
; ktoré musia byť splnené pre správne fungovanie programu.

; PODMIENKY pre správne fungovanie programu:
; - vstupný súbor musí mať max. 80 znakov na jeden riadok 
	; je to z dôvodu čitateľnosti a prehľadnosti výstupu v termináli
; - vstupný súbor nesmie obsahovať špeciálny znak '$'
	; - je to z dôvodu, že to je špeciálny znak na ukončenie reťazca
; - vstupný súbor môže mať názov max. 16 znakov
	; - sám DOS podporuje max. 8 znakov pre názov súboru, čiže ide iba o rezervu
; - vstupný súbor musí mať vždy za bodku na konci vety medzeru aby to bola korektne
;	napísaná veta a tým pádom sa počítalo slovo začínajúce veľkým písmenom (ak existuje)