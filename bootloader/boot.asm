[BITS 16]           ; Modo real (BIOS trabaja en modo real)
[ORG 0x7C00]       ; La BIOS carga el código en la dirección 0x7C00

start:
    mov si, msg   ; Cargar la dirección del mensaje en SI
    call print    ; Llamar a la función que imprime

hang:
    jmp hang      ; Bucle infinito para detener el programa

; Función para imprimir un mensaje
print:
    mov ah, 0x0E  ; BIOS: función de impresión de caracteres en modo texto
.loop:
    lodsb         ; Cargar siguiente byte de la cadena en AL
    cmp al, 0     ; ¿Es el fin de la cadena?
    je done       ; Si sí, salir
    int 0x10      ; Llamar a la BIOS para imprimir el carácter
    jmp .loop     ; Repetir hasta terminar
done:
    ret           ; Regresar

msg db "Hello, si funciono!", 0  ; Mensaje que se imprimirá en pantalla

times 510-($-$$) db 0  ; Rellenar hasta 510 bytes con ceros
dw 0xAA55              ; Firma de arranque (MBR)
