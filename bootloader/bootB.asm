bits 16
org 0x7C00

start:
    mov si, msg_loading  ; Mostrar mensaje de carga
    call print

    
    call read_sector     ; Leer sector 2 desde el disco

    jmp 0x9000           ; Saltar a la dirección cargada (ejecutar código)

hang:
    jmp hang             ; Bucle infinito

; -------------------
; Función imprimir
; -------------------
print:
    mov ah, 0x0E
.loop:
    lodsb
    cmp al, 0
    je done
    int 0x10
    jmp .loop
done:
    ret

; -------------------
; Leer sector 2 desde el disco
; -------------------
read_sector:
    mov ah, 0x02        ; Función de lectura de disco
    mov al, 1           ; Leer 1 sector
    mov ch, 0           ; Cilindro 0
    mov cl, 2           ; Sector 2
    mov dh, 0           ; Cabeza 0
    mov dl, 0x80        ; Disco (USB)
    mov ax, 0x9000   ; Cargar segmento donde se guardará el sector
    mov es, ax       ; ES ahora apunta a 0x9000
    mov bx, 0x0000   ; Dirección dentro del segmento
    int 0x13           
                     ; Si hay error, saltar a mostrar mensaje
    mov si, msg_loading   ; Mensaje de éxito
    call print       ; Imprimir mensaje si la lectura fue exitosa
    jmp 0x9000
    mov si, msg_ok        ; Saltar al código cargado en memoria
    call print 
    jc errorB            ; Si hay error, mostrar mensaje
    ret

errorA:
    mov si, msg_error_a
    call print
    mov ah, 0x0E    ; Modo texto
    mov al, ' '     ; Espacio antes del código
    int 0x10

    mov al, ah      ; Cargar código de error en AL
    call print_hex  ; Imprimir en pantalla
    jmp hang

errorB:
    mov si, msg_error_b
    call print
    mov ah, 0x0E    ; Modo texto
    mov al, ' '     ; Espacio antes del código
    int 0x10

    mov al, ah      ; Cargar código de error en AL
    call print_hex  ; Imprimir en pantalla
    jmp hang

print_hex:
    pusha
    mov cl, 4       ; 4 bits por cada dígito hexadecimal
.hex_loop:
    rol al, cl      ; Mover el siguiente dígito a la izquierda
    mov bl, al
    and bl, 0x0F    ; Aislar los últimos 4 bits
    cmp bl, 10
    jl .num
    add bl, 'A' - 10
    jmp .print
.num:
    add bl, '0'
.print:
    mov al, bl
    mov ah, 0x0E
    int 0x10
    loop .hex_loop
    popa
    ret

msg_loading db "Cargando sector 2...", 0
msg_error_a   db "Error al leer sector! A", 0
msg_error_b  db "Error al leer sector! B", 0
msg_ok db "Sector 2 leido correctamente!", 0

times 510-($-$$) db 0
dw 0xAA55
