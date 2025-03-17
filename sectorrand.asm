BITS 16
[ORG 0]          ; ORG 0 porque el código se carga en DS:0 = 0x7E00

start:
    ; Configuramos DS para que DS:0 = 0x7E00
    mov ax, 0x7E0
    mov ds, ax
    mov bx, 0x0000
    
    mov si, newline_msg
    call print_string

    

    call read_random_sector  ; Leer un sector del disco
    call extract_random_byte ; Extraer un número pseudoaleatorio
    
    call print_number        ; Imprimir el número generado
    
    jmp $                   ; Bucle infinito

; --------------------------------
; Función: Leer un sector del disco (LBA 2 → CHS: Cil 0, Cabeza 0, Sector 3)
; --------------------------------
read_random_sector:
    mov ah, 0x02      ; Función de lectura de disco (BIOS INT 13h)
    mov al, 1         ; Leer 1 sector (512 bytes)
    mov ch, 0         ; Cilindro 0
    mov cl, 3         ; Sector 3 (LBA 2 en CHS)
    mov dh, 0         ; Cabeza 0
    mov dl, 0x80      ; Disco duro (o 0x00 si es disquete)
    
    mov ax, buffer    ; Carga el segmento donde está 'buffer'
    mov es, ax            ; ES = segmento
    mov bx, buffer ; BX = offset

  
    int 0x13          ; Llamar a la BIOS para leer
    jc disk_error     ; Si hay error, mostrar mensaje
    
    ret

disk_error:
    mov si, error_msg
    call print_string
    jmp $

; --------------------------------
; Función: Extraer un byte de los datos leídos y usarlo como número aleatorio
; --------------------------------
extract_random_byte:
    mov al, [buffer]  ; Tomamos el primer byte del sector leído
    cbw               ; Extender a 16 bits (AX = AL)
    ret

; --------------------------------
; Función: Imprimir el número almacenado en AX
; --------------------------------
print_number:
    mov cx, 0        ; Contador de dígitos
    mov bx, 10       ; Base 10
    cmp ax, 0
    jne .convert_loop; Si es 0, imprime '0'
    mov al, '0'
    mov ah, 0x0E
    int 0x10

.convert_loop:
    xor dx, dx       ; Limpiar DX
    div bx           ; AX / 10 → Cociente en AX, residuo en DX
    push dx          ; Guardamos dígito en la pila
    inc cx           ; Contamos cuántos dígitos hay
    test ax, ax      ; Si AX == 0, terminamos
    jnz .convert_loop  

.print_loop:
    pop dx           ; Recuperar el último dígito
    add dl, '0'      ; Convertir a ASCII
    mov ah, 0x0E     ; Función de impresión de INT 0x10
    int 0x10         ; Imprimir carácter
    loop .print_loop  

    ret

; --------------------------------
; Función: Imprimir una cadena de caracteres
; --------------------------------
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

error_msg db "Error al leer el disco!", 0
newline_msg db 0x0D, 0x0A, 0
input_msg db "obteniendo sector ", 0

buffer: times 512 db 0  ; Buffer para almacenar los datos leídos del disco
