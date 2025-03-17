BITS 16
ORG 0x7E00          ; ORG 0 porque el código se carga en DS:0 = 0x7E00

start:
    
    ;=========imprimir instrucciones
    mov bx, newline_msg
    call print_string


    jmp game

    mov bx, newline_msg
    call print_string

    
    ;mov ax,5
    ;mov bx,6
    ;cmp ax,bx
    ;jle comparacion

    ;mov bx, phonetic_alphabet
    ;add bx, 179
    ;call print_string

    ; Imprimir nueva línea
    mov bx, newline_msg
    call print_string

    

    ;mov bx, 
    ;call print_string
    jmp $

;===============================
;loop del juego
game:
    ;=======================aqui se necesita generar la palabra
    ; Mostrar mensaje de solicitud
    mov bx, newline_msg
    call print_string
    mov bx, prompt_msg
    call print_string
    mov bx, palabra_de_prueba
    call print_string

    mov bx, newline_msg
    call print_string  
.spell_loop:
    call read_string
    mov bx, newline_msg
    call print_string
    ; Mostrar mensaje indicando lo ingresado
    mov bx, input_msg
    call print_string
    
    ; Imprimir lo ingresado (cadena almacenada en buffer)
    mov bx, buffer
    call print_string

    ; Imprimir nueva línea
    mov bx, newline_msg
    call print_string
    call cls

    call check_B
    jmp $
    ret

;================================
;rutina para comparar la palabra ingresada

check_word:

    lea cx, buffer
    sub cx, 97
    mov bx, phonetic_alphabet  ; Dirección base
    mov ax, 9                 ; Tamaño de cada palabra (9 bytes)
    mul cx                     ; Multiplicamos índice * tamaño
    add bx, ax 

    call print_string
    
.check_loop:
    ret    

checkA:
mov si, phonetic_alphabet  ; Cargar dirección de inicio

next_word:
    cmp byte [si], bl
    je found_word

find_next:
    lodsb                  ; Cargar byte en AL y avanzar SI
    cmp al, 0              ; ¿Es el final de la palabra?
    jne find_next          ; Si no, seguir avanzando

    cmp byte [si], 0       ; ¿Es el final de la lista?
    je end_list                ; Si sí, terminar

    jmp next_word          ; Ir a la siguiente palabra

found_word:
    mov bx, si
    call print_string      ; Imprimir la palabra actual
    mov bx, newline_msg
    call print_string 
end_list:
    ret

check_B:
    mov di, buffer
    mov bl, [di]
    
    cmp bl, "a"
    je letraA

letraA:
    mov bl, 'A'
    jmp checkA
jmp $
;===========================================000
;===========================================000

checkB:
mov si, phonetic_alphabet  ; Cargar dirección de inicio

next_wordB:
    mov bx, si
    call print_string      ; Imprimir la palabra actual
    mov bx, newline_msg
    call print_string          ; Nueva línea después de imprimir

    ; Buscar el siguiente 0x00
find_nextB:
    lodsb                  ; Cargar byte en AL y avanzar SI
    cmp al, 0              ; ¿Es el final de la palabra?
    jne find_nextB          ; Si no, seguir avanzando

    cmp byte [si], 0       ; ¿Es el final de la lista?
    je doneB                ; Si sí, terminar

    jmp next_wordB          ; Ir a la siguiente palabra

doneB:
    ret
;================================


cls:
  pusha
  mov ah, 0x00
  mov al, 0x03  ; text mode 80x25 16 colours
  int 0x10
  popa
  ret

;================================
; Rutina para imprimir cadenas (usa INT 10h, AH=0x0E)

print_string:                   ;cx = string length                                                                                                                                                          
    mov ah, 0x0E
.print_loop:                                                                                                                                                   
    mov al, [bx]
    int 0x10
    cmp al, 0
    je .done
    inc bx
    jmp .print_loop
.done:
    xor al, al
    xor bx, bx
    ret

;===============================
;Rutina para sumar numeros
;el numero debe estar iniciado como caracter
;mov ax, [size] mover a ax el numero
    sub ax, '0'
    add ax, 1
    add ax, '0'
;mov [size], ax devolver el numero a donde esta guardado


;====================================
; Rutina para leer una cadena del teclado.
; Lee carácter a carácter hasta que se presione ENTER (0x0D) y hace eco.
read_string:
    mov di, buffer  ; DI apunta al buffer
.read_loop:
    mov ah, 0       ; Espera una tecla (INT 16h)
    int 0x16
    cmp al, 0x0D   ; ¿Se presionó ENTER?
    je .done_read
    ; Eco: imprimir el carácter leído
    mov ah, 0x0E
    int 0x10
    ; Guardar el carácter en el buffer
    mov [di], al
    inc di
    jmp .read_loop
.done_read:
    mov byte [di], 0   ; Terminar la cadena con 0
    xor al, al
    ret

read_string_B:
    mov di, buffer  ; DI apunta al buffer
.read_loop_B:
    mov ah, 0       ; Espera una tecla (INT 16h)
    int 0x16
    cmp al, 0x0D   ; ¿Se presionó ENTER?
    je .done_read_B
    ; Guardar el carácter en el buffer
    mov [di], al
    inc di
    jmp .read_loop_B
.done_read_B:
    mov byte [di], 0   ; Terminar la cadena con 0
    ret

;===================================
; Mensajes y buffer

error db "sin coincidencia", 0
prompt_msg db "Palabra a deletrear: ", 0
newline_msg db 0x0D, 0x0A, 0
input_msg db "Lo que ingreso fue: ", 0
buffer      times 128 db 0
size db '5', 0
palabra_de_prueba db "marco", 0

prompt db "Ingrese una letra (A-Z): ", 0x0D, 0x0A, 0x00
invalid_msg db "Letra invalida!", 0x0D, 0x0A, 0x00
phonetic_alphabet:
    db "Alpha", 0x00
    db "Bravo", 0x00
    db "Charlie", 0x00
    db "Delta", 0x00
    db "Echo", 0x00
    db "Foxtrot", 0x00
    db "Golf", 0x00
    db "Hotel", 0x00
    db "India", 0x00
    db "Juliett", 0x00
    db "Kilo", 0x00
    db "Lima", 0x00
    db "Mike", 0x00
    db "November", 0x00
    db "Oscar", 0x00
    db "Papa", 0x00
    db "Quebec", 0x00
    db "Romeo", 0x00
    db "Sierra", 0x00
    db "Tango", 0x00
    db "Uniform", 0x00
    db "Victor", 0x00
    db "Whiskey", 0x00
    db "X-ray", 0x00
    db "Yankee", 0x00
    db "Zulu", 0x00, 0x00

phonetic_pointers:
    dd phonetic_alphabet+0
    dd phonetic_alphabet+9
    dd phonetic_alphabet+17
    dd phonetic_alphabet+26
    dd phonetic_alphabet+35
    dd phonetic_alphabet+44
    dd phonetic_alphabet+53
    dd phonetic_alphabet+62
    dd phonetic_alphabet+71
    dd phonetic_alphabet+80
    dd phonetic_alphabet+89
    dd phonetic_alphabet+98
    dd phonetic_alphabet+107
    dd phonetic_alphabet+116
    dd phonetic_alphabet+125
    dd phonetic_alphabet+134
    dd phonetic_alphabet+143
    dd phonetic_alphabet+152
    dd phonetic_alphabet+161
    dd phonetic_alphabet+170
    dd phonetic_alphabet+179
    dd phonetic_alphabet+188
    dd phonetic_alphabet+197
    dd phonetic_alphabet+206
    dd phonetic_alphabet+215
    dd phonetic_alphabet+224


