BITS 16
ORG 0x7C00  ; Dirección donde la BIOS carga el sector de arranque

start:
    ; Mostrar mensaje de inicio
    mov si, boot_msg
    call print_string


    mov ax, 0x7E0   ; Segmento donde se carga el programa
    mov es, ax
    mov bx, 0x0000  ; Offset 0x0000 dentro de ES:BX

    mov ah, 0x02    ; Función de INT 13h para leer desde el disco
    mov al, 2       ; Número de sectores a leer (1 sector = 512 bytes)
    mov ch, 0       ; Cilindro 0
    mov cl, 2       ; Sector 2 (LBA 1 en discos CHS)
    mov dh, 0       ; Cabeza 0
    mov dl, 0x80    ; Disco duro (o 0x00 si es disquete)
    int 0x13        ; Llamar a la BIOS para leer el sector

    jc disk_error   ; Si hay error, mostrar mensaje

    ; Saltar a ejecutar el código cargado en 0x7E00:0000
    jmp 0x7E0:0x0000

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

boot_msg db "Bootloader iniciado. Cargando MRPV...", 0
error_msg db "Error al iniciar! boot", 0

times 510-($-$$) db 0  ; Rellenar hasta 510 bytes
dw 0xAA55              ; Firma de sector de arranque