BITS 16
ORG 0x7E0

start:
    ; Obtener la hora del sistema
    mov ah, 0x02    ; Función para leer la hora
    int 0x1A        ; Llamar a la BIOS
    mov al, ch      ; Cargar la hora en AL
    call bin_to_ascii
    mov al, cl      ; Cargar los minutos en AL
    call bin_to_ascii
    mov al, dh      ; Cargar los segundos en AL
    call bin_to_ascii

    ; Imprimir el buffer con la hora formateada
    mov si, buffer
    call print_string

    jmp $

; --- Convierte número binario en ASCII ---
bin_to_ascii:
    mov ah, 0       ; Limpiar AH
    mov bl, 10      ; Divisor 10
    div bl          ; AX / 10, AL = cociente, AH = residuo
    add al, '0'     ; Convertir AL a ASCII
    stosb           ; Guardar en el buffer
    mov al, ah      ; AL = residuo (segunda cifra)
    add al, '0'     ; Convertir a ASCII
    stosb           ; Guardar en el buffer
    mov al, ':'     ; Agregar separador
    stosb
    ret

; --- Imprimir cadena en pantalla ---
print_string:
    mov ah, 0x0E  ; Función para imprimir caracter
.loop:
    lodsb         ; Cargar siguiente byte del buffer
    cmp al, 0     ; ¿Fin de la cadena?
    je .done      ; Si es 0, terminar
    int 0x10      ; Imprimir caracter
    jmp .loop     ; Repetir
.done:
    ret

buffer times 20 db 0  ; Espacio para almacenar la hora

times 512-($-$$) db 0  ; Rellenar hasta 510 bytes
            ; Firma de boot
