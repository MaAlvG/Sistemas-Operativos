[BITS 16]
[ORG 0]          ; ORG 0 porque el código se carga en DS:0 = 0x7E00

start:
    ; Configuramos DS para que DS:0 = 0x7E00
    mov ax, 0x7E0
    mov ds, ax

    ; Mostrar mensaje de solicitud
    mov si, prompt_msg
    call print_string

    ; Leer la cadena desde el teclado
    call read_string

    ; Imprimir nueva línea
    mov si, newline_msg
    call print_string

    ; Mostrar mensaje indicando lo ingresado
    mov si, input_msg
    call print_string

    ; Imprimir lo ingresado (cadena almacenada en buffer)
    mov si, buffer
    call print_string

    jmp $

; Rutina para imprimir cadenas (usa INT 10h, AH=0x0E)
print_string:
    mov ah, 0x0E
.print_loop:
    lodsb           ; Carga el siguiente byte en AL usando DS:SI
    cmp al, 0
    je .done
    int 0x10        ; Imprime el carácter
    jmp .print_loop
.done:
    ret

; Rutina para leer una cadena del teclado.
; Lee carácter a carácter hasta que se presione ENTER (0x0D) y hace eco.
read_string:
    mov di, buffer  ; DI apunta al buffer
.read_loop:
    mov ah, 0       ; Espera una tecla (INT 16h)
    int 0x16
    cmp al, 0x0D   ; ¿Se presionó ENTER?
    je .done_read
    ; Eco: imprimir el carácter leído
    mov ah, 0x0E
    int 0x10
    ; Guardar el carácter en el buffer
    mov [di], al
    inc di
    jmp .read_loop
.done_read:
    mov byte [di], 0   ; Terminar la cadena con 0
    ret

; Mensajes y buffer
prompt_msg db "Ingrese una palabra: ", 0
newline_msg db 0x0D, 0x0A, 0
input_msg db "Lo que ingreso fue: ", 0
buffer      times 128 db 0

; Rellenar hasta 512 bytes
times 512 - ($-$$) db 0
