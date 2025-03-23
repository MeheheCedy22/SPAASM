; --------------------- Macros ---------------------
; Prints a dollar-terminated string to the standard output.
; str_name - Name of the string variable to print.
print_str Macro str_name
	mov dx, offset str_name     ; Load the address of str_name first char into DX
	call str_print_service      ; Call the string-printing procedure
endm

; Opens a file for reading and handles errors.
; filename - Name of the file to open.
open_file Macro filename
	mov ah, 3dh 				; DOS service - open file according to access mode in AL
	mov al, 0 					; AL: 0-read only mode
	mov dx, offset filename 	; DX points to filename in data segment which stores the filename provided as an argument to the program
	int 21h                     ; Call DOS interrupt
	jc file_open_error          ; Jump to error handler if CF is set (error occurred)
endm 							; After sucess, file handle is in AX

; Closes an open file.
; file_handle - Handle of the file to close.
close_file Macro file_handle
	mov ah, 3eh 			    ; DOS service - close file
	mov bx, file_handle 	    ; BX must contain the file handle for the DOS service
	int 21h                     ; Call DOS interrupt
endm

; Converts a numeric value to ASCII and prints it.
; variable_name - Name of the numeric variable to convert and print.
; - this macro is heavily insipired by the code i found on the internet
print_number Macro variable_name
	push ax                     ; Preserve registers
	push bx
	push cx
	push dx
	push di
	
	; Set up for conversion
	mov ax, variable_name       ; Load the number to convert
	mov bx, 10                  ; Divisor (decimal base)
	mov di, offset number_buffer
	add di, 9                   ; Point to the end of the buffer (before '$')
	mov cx, 0                   ; Counter for number of digits
	
	; Special case for zero
	test ax, ax                 ; Check if the number is zero
	jnz convert_loop            ; If not zero, proceed with conversion
	mov byte ptr [di], '0'      ; Otherwise, store '0' character
	dec di                      ; Move buffer pointer left
	inc cx                      ; Increment digit counter
	jmp print_it                ; Skip conversion loop
	
convert_loop:
	xor dx, dx                  ; Clear DX for division (ensures high word is 0)
	div bx                      ; AX = quotient, DX = remainder
	add dl, '0'                 ; Convert remainder to ASCII
	mov [di], dl                ; Store digit in buffer
	dec di                      ; Move buffer pointer left
	inc cx                      ; Increment digit counter
	test ax, ax                 ; Check if quotient is zero
	jnz convert_loop            ; Continue if not zero
	
print_it:
	; DI now points one position before first digit
	inc di                      ; Point to first digit
	mov dx, di                  ; Set DX for print service
	call str_print_service      ; Print the converted number
	
	pop di                      ; Restore registers
	pop dx
	pop cx
	pop bx
	pop ax
endm
; --------------------- END Macros ---------------------