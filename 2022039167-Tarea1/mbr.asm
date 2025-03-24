[BITS 16]
[ORG 0x7C00]  ; Dirección donde la BIOS carga el sector de arranque

start:
    mov si, boot_msg
    call print_string


    mov ax, 0x7E0   ; Segmento donde se carga el programa
    mov es, ax
    mov bx, 0x0000  

    mov ah, 0x02    ; carga la función de INT 13h para leer desde el disco
    mov al, 3       ; Número de sectores a leer 
    mov ch, 0       ; Cilindro 0
    mov cl, 2       ; Sector 2 
    mov dh, 0       ; Cabeza 0
    mov dl, 0x80    ; Disco duro (o 0x00 si es disquete)
    int 0x13        ; Llamar a la BIOS para leer el sector

    jc disk_error   ; Si hay error, mostrar mensaje

    jmp 0x7E0:0x0000 ; Saltar a ejecutar el código 

disk_error:
    mov si, error_msg
    call print_string
    jmp $

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

boot_msg db "Bootloader iniciado. Cargando Juego...", 0
error_msg db "Error al bootear", 0

times 510-($-$$) db 0  ; Rellenar hasta 510 bytes
dw 0xAA55              ; Firma de sector de arranque