bits 16
org 0x9000     ; Se ejecutará en la dirección 0x9000

start:
    mov si, msg
    call print

hang:
    jmp hang    ; Bucle infinito

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

msg db "Hello, World! desde adentro", 0

times 512-($-$$) db 0  ; Asegurar que ocupa exactamente 1 sector
