[BITS 16]
[ORG 0]   ; Dirección en la que el MBR carga este código

start:
    ; Configuramos DS para acceder a los datos (usamos el mismo segmento que el código)
    mov ax, 0x7E0
    mov ds, ax

    mov si, newline_msg
    call print_string

    ; Mostrar mensaje de solicitud de entrada
    mov si, prompt_msg
    call print_string

    mov si, newline_msg
    call print_string

    ; Leer la cadena de teclado y guardarla en buffer
    call read_string

    ; Imprimir nueva línea
    mov si, newline_msg
    call print_string

    ; Mostrar mensaje indicando lo ingresado
    mov si, input_msg
    call print_string

    ; Imprimir lo ingresado (la cadena almacenada en buffer)
    mov si, buffer
    call print_string

    jmp $

; Rutina para imprimir una cadena de caracteres
print_string:
    mov ah, 0x0E      ; Modo de impresión en teletipo
.print_loop:
    lodsb             ; Cargar siguiente carácter (AL) desde DS:SI
    cmp al, 0         ; Fin de cadena?
    je .done
    int 0x10          ; Imprimir carácter en pantalla
    jmp .print_loop
.done:
    ret

; Rutina para leer una cadena del teclado.
; Lee carácter por carácter hasta que se presiona ENTER (0x0D).
; Los caracteres se almacenan en "buffer" y al finalizar se agrega un 0.
read_string:
    mov di, buffer    ; DI apunta al buffer donde se guardará la cadena
.read_loop:
    mov ah, 0         ; Espera a que se presione una tecla
    int 0x16
    cmp al, 0x0D      ; ¿Se presionó ENTER?
    je .done_read
    ; Eco: imprimir el carácter leído
    mov ah, 0x0E
    int 0x10
    ; Guardar el carácter en el buffer
    mov [di], al
    inc di
    jmp .read_loop
.done_read:
    ; Agregar terminador nulo al final de la cadena
    mov byte [di], 0
    ret

prompt_msg db "Ingrsese una palabra: ", 0
newline_msg db 0x0D, 0x0A, 0
input_msg db "Lo que ingreso fue: ", 0
buffer      times 128 db 0   ; Buffer para almacenar la entrada del usuario

      ; Rellenar hasta 512 bytes
