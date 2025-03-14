[BITS 16]
[ORG 0x7E00]  ; La dirección donde el MBR carga este código

start:
    mov si, hello_msg
    call print_string

    jmp $

print_string:
    mov ah, 0x0E  ; Función de INT 10h para imprimir caracteres en modo texto
.loop:
    lodsb         ; Cargar el siguiente carácter en AL
    cmp al, 0     ; ¿Es el final de la cadena?
    je .done
    int 0x10      ; Imprimir el carácter
    jmp .loop
.done:
    ret

hello_msg db "Hello, World from LBA 1!", 0

times 512-($-$$) db 0  ; Rellenar hasta 512 bytes
