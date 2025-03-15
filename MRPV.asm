[BITS 16]
[ORG 0x7E00]

start:
    mov ax, 0x7E0
    mov ds, ax

    ; Generar longitud aleatoria (entre 4 y 10)
    call get_random
    and al, 0x07       ; Limita a valores entre 0 y 7
    add al, 4          ; Ajusta para que esté entre 4 y 10
    mov [str_len], al  ; Guardar longitud generada

    ; Generar la cadena aleatoria
    mov cx, ax         ; CX = longitud de la cadena
    mov di, random_str ; DI apunta al buffer de la cadena
gen_loop:
    call get_random
    and al, 0x1F       ; Limita a valores entre 0 y 31
    add al, 'a'        ; Ajusta para estar en el rango 'a' - 'z'
    cmp al, 'z'        ; Asegurar que no se pase del rango
    jg gen_loop
    mov [di], al       ; Almacena carácter en la cadena
    inc di
    loop gen_loop

    mov byte [di], 0   ; Agregar terminador nulo

    ; Mostrar la cadena generada
    mov si, prompt_msg
    call print_string
    mov si, random_str
    call print_string

    jmp $

; Generar un número pseudoaleatorio en AL usando el temporizador
get_random:
    mov ah, 0          ; Función 00h de INT 1Ah: Obtener ticks desde medianoche
    int 0x1A
    xor ah, ah         ; Usar AX como número aleatorio
    ret

; Imprime una cadena terminada en 0
print_string:
    mov ah, 0x0E
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

prompt_msg db "Cadena generada: ", 0
random_str times 11 db 0   ; Espacio para la cadena aleatoria (máx. 10 caracteres + terminador)
str_len db 0               ; Almacena la longitud generada

times 512 - ($-$$) db 0  ; Relleno hasta 512 bytes
