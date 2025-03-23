; --------------------- MACROS ---------------------
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


print_number Macro variable_name
    push ax
    push bx
    push cx
    push dx
    push di
    
    ; Set up for conversion
    mov ax, variable_name   ; Load the number to convert
    mov bx, 10              ; Divisor (decimal base)
    mov di, offset number_buffer
    add di, 9               ; Point to the end of the buffer (before '$')
    mov cx, 0               ; Counter for number of digits
    
    ; Special case for zero
    test ax, ax
    jnz convert_loop
    mov byte ptr [di], '0'
    dec di
    inc cx
    jmp print_it
    
convert_loop:
    xor dx, dx              ; Clear DX for division
    div bx                  ; AX = quotient, DX = remainder
    add dl, '0'             ; Convert remainder to ASCII
    mov [di], dl            ; Store digit
    dec di                  ; Move buffer pointer left
    inc cx                  ; Increment digit counter
    test ax, ax             ; Check if quotient is zero
    jnz convert_loop      ; Continue if not zero
    
print_it:
    ; DI now points one position before first digit
    inc di                  ; Point to first digit
    mov dx, di              ; Set DX for print service
    call str_print_service
    
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
endm
; --------------------- END MACROS ---------------------