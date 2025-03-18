BITS 16
ORG 0x7E00          ; ORG 0 porque el código se carga en DS:0 = 0x7E00

start:
    
    ;=========imprimir instrucciones
    
    mov bx, newline_msg
    call print_string

    jmp game

    mov bx, newline_msg
    call print_string


    ; Imprimir nueva línea
    mov bx, newline_msg
    call print_string

    ;mov bx, 
    ;call print_string

        ; Mostrar mensaje indicando lo ingresado
    ;mov bx, input_msg
    ;call print_string
    
    ; Imprimir lo ingresado (cadena almacenada en buffer)
    ;mov bx, buffer
    ;call print_string

    ; Imprimir nueva línea
    ;mov bx, newline_msg
    ;call print_string
    
    jmp $

;===============================
;loop del juego
game:

    mov bx, newline_msg
    call print_string

    mov bx, prompt_msg
    call print_string

    mov bx, palabra_de_prueba
    call print_string

    mov bx, newline_msg
    call print_string 
    call generate_word
    mov bx, newline_msg
    call print_string 

.spell_loop:
    mov di, buffer1
    call read_string
    mov bx, newline_msg
    call print_string 

    mov di, buffer2
    call read_string
    mov bx, newline_msg
    call print_string

    mov di, buffer3
    call read_string
    mov bx, newline_msg
    call print_string

    mov di, buffer4
    call read_string
    mov bx, newline_msg
    call print_string 

    mov di, buffer5
    call read_string
    mov bx, newline_msg
    call print_string 
    ;mov byte [di], 0 

get_punctuation:
    mov di, user_spelling
    mov bx, buffer1
    call copy_word
    mov bx, buffer2
    call copy_word
    mov bx, buffer3
    call copy_word
    mov bx, buffer4
    call copy_word
    mov bx, buffer5
    call copy_word
    mov byte [di], 0

    mov bx, user_spelling
    call print_string

mov dx, 0            ; Inicializar puntuación en 0
mov si, phonetic_spelling
mov di, user_spelling

compare_letters:
    mov al, [si]     ; Letra original
    mov bl, [di]     ; Letra ingresada
    cmp al, 0
    je show_score

    call update_score  ; Actualizar puntaje

    inc si            ; Siguiente letra
    inc di
    jmp compare_letters

update_score:
    cmp al, bl              ; Comparar letras originales con las ingresadas
    je .correct             ; Si son iguales, sumar puntos
    cmp al, '*'             ; Si es '*', restar más puntos
    je .incorrect_star

.incorrect:
    sub dx, 1               ; Restar 1 punto por error
    ret

.incorrect_star:
    sub dx, 2               ; Restar 2 puntos si fue '*'
    ret

.correct:
    add dx, 2               ; Sumar 2 puntos si fue correcto
    ret

show_score:
    call convert_to_string
    mov bx, number_buffer
    call print_string
    jmp $  ; Mostrar puntaje final (puedes implementar esto)
    ret


;================================
generate_word:
    ;=======================aqui se necesita generar la palabra
    ; Mostrar mensaje de solicitud
    call spell_word

    ret

spell_word:
    mov si, palabra_de_prueba   ; SI apunta a la palabra de entrada
    mov di, phonetic_spelling   ; DI apunta al user_spelling de salida

next_letter_to_spell:
    mov bl, [si]         ; Cargar la letra actual en BL
    cmp bl, 0            ; ¿Fin de la palabra?
    je .done_spelling

    call check_B         ; Buscar la palabra fonética (debe guardar dirección en BX)
    call copy_word       ; Copiar la palabra fonética en `phonetic_spelling`

    inc si               ; Avanzar al siguiente carácter de la palabra original
    jmp next_letter_to_spell

.done_spelling:
    mov byte [di], 0     ; Agregar terminador nulo
    mov bx, phonetic_spelling
    call print_string
    ret                  ; Termina la función

;==============================================
;Rutina para copiar una palabra en otro espacio
copy_word:
    cld                  ; Asegurar incremento normal
.copy_loop:
    mov al, [bx]         ; Cargar byte desde la palabra fonética
    cmp al, 0            ; ¿Fin de la palabra?
    je .done_copy
    mov [di], al         ; Guardar byte en destino
    inc di               ; Avanzar en el destino
    inc bx               ; Avanzar en la palabra de origen
    jmp .copy_loop

.done_copy:
    ret

;================================
;rutina para guardar un numero como cadena de caracteres
convert_to_string:
    mov bx, 10           ; Divisor para separar los dígitos (base 10)
    mov si, number_buffer ; Apuntamos a un buffer vacío para almacenar la cadena
    add si, 10           ; Mover a la última posición del buffer (espacio para 10 dígitos)
    mov byte [si], 0     ; Poner el terminador de cadena (0) en la última posición

convert_loop:
    dec si                ; Moverse hacia atrás en el buffer
    xor dx, dx            ; Limpiar el registro DX (para el residuo)
    div bx                ; DX:AX / 10 -> AX contiene el cociente, DX contiene el residuo
    add dl, '0'           ; Convertir el residuo (número) a su valor ASCII
    mov [si], dl          ; Almacenar el carácter ASCII en el buffer

    cmp ax, 0             ; Si el cociente es 0, hemos terminado
    je .done_convert

    jmp convert_loop

.done_convert:
    ret



;======================================
check_B:

    cmp bl, "a"
    je letraA

    cmp bl, "b"
    je letraB
    
    cmp bl, "c"
    je letraC
    
    cmp bl, "d"
    je letraD
    
    cmp bl, "e"
    je letraE
    
    cmp bl, "f"
    je letraF
    
    cmp bl, "g"
    je letraG
    
    cmp bl, "h"
    je letraH
    
    cmp bl, "i"
    je letraI
    
    cmp bl, "j"
    je letraJ
    
    cmp bl, "k"
    je letraK
    
    cmp bl, "l"
    je letraL
    
    cmp bl, "m"
    je letraM
    
    cmp bl, "n"
    je letraN
    
    cmp bl, "o"
    je letraO
    
    cmp bl, "p"
    je letraP
    
    cmp bl, "q"
    je letraQ
    
    cmp bl, "r"
    je letraR
    
    cmp bl, "s"
    je letraS
    
    cmp bl, "t"
    je letraT
    
    cmp bl, "u"
    je letraU
    
    cmp bl, "v"
    je letraV
    
    cmp bl, "w"
    je letraW
    
    cmp bl, "x"
    je letraX
    
    cmp bl, "y"
    je letraY
    
    cmp bl, "z"
    je letraZ

    jmp letraInvalida

letraA:
    mov bx, alpha
    ret
letraB:
    mov bx, bravo
    ret
letraC:
    mov bx, charlie
    ret
letraD:
    mov bx, delta
    ret
letraE:
    mov bx, echo
    ret
letraF:
    mov bx, foxtrot
    ret
letraG:
    mov bx, golf
    ret
letraH:
    mov bx, hotel
    ret
letraI:
    mov bx, india
    ret
letraJ:
    mov bx, juliett
    ret
letraK:
    mov bx, kilo
    ret
letraL:
    mov bx, lima
    ret
letraM:
    mov bx, mike
    ret
letraN:
    mov bx, november
    ret
letraO:
    mov bx, oscar
    ret
letraP:
    mov bx, papa
    ret
letraQ:
    mov bx, quebec
    ret
letraR:
    mov bx, rome
    ret
letraS:
    mov bx, sierra
    ret
letraT:
    mov bx, tango
    ret
letraU:
    mov bx, uniform
    ret
letraV:
    mov bx, victor
    ret
letraW:
    mov bx, whiskey
    ret
letraX:
    mov bx, xray
    ret
letraY:
    mov bx, yankee
    ret
letraZ:
    mov bx, zulu
    ret
letraInvalida:
    mov bx, asterisco
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
; Rutinas para imprimir cadenas (usa INT 10h, AH=0x0E)

print_string:                   ;cx = string length                                                                                                                                                          
    mov ah, 0x0E
.print_loop:                                                                                                                                                   
    mov al, [bx]
    cmp al, 0
    je .done
    int 0x10
    inc bx
    jmp .print_loop
.done:
    xor al, al
    xor bx, bx
    ret

print_string_si:
    mov ah, 0x0E
.loop_si:
    lodsb
    cmp al, 0
    je .done_si
    int 0x10
    jmp .loop_si
.done_si:
    ret
;===============================
;Rutina para sumar numeros
;el numero debe estar iniciado como caracter
;mov ax, [points] mover a ax el numero
    sub ax, '0'
    add ax, 1
    add ax, '0'
;mov [points], ax devolver el numero a donde esta guardado


;====================================
; Rutina para leer una cadena del teclado.
; Lee carácter a carácter hasta que se presione ENTER (0x0D) y hace eco.
read_string:
      ; DI apunta al buffer
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
    mov di, buffer1  ; DI apunta al buffer
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
asterisco db "*", 0
alpha db "Alpha", 0
bravo   db "Bravo", 0x00
charlie    db "Charlie", 0x00
delta    db "Delta", 0x00
echo    db "Echo", 0x00
foxtrot    db "Foxtrot", 0x00
golf    db "Golf", 0x00
hotel    db "Hotel", 0x00
india    db "India", 0x00
juliett    db "Juliett", 0x00
kilo    db "Kilo", 0x00
lima    db "Lima", 0x00
mike    db "Mike", 0x00
november    db "November", 0x00
oscar    db "Oscar", 0x00
papa    db "Papa", 0x00
quebec    db "Quebec", 0x00
rome    db "Romeo", 0x00
sierra    db "Sierra", 0x00
tango    db "Tango", 0x00
uniform    db "Uniform", 0x00
victor    db "Victor", 0x00
whiskey    db "Whiskey", 0x00
xray    db "X-ray", 0x00
yankee    db "Yankee", 0x00
zulu    db "Zulu", 0x00

error db "sin coincidencia", 0

prompt_msg db "Palabra a deletrear: ", 0

newline_msg db 0x0D, 0x0A, 0

input_msg db "Lo que ingreso fue: ", 0

points db '5', 0

palabra_de_prueba db "marco", 0

prompt db "Ingrese una letra (A-Z): ", 0x0D, 0x0A, 0x00

invalid_msg db "Letra invalida!", 0x0D, 0x0A, 0x00

user_spelling              times 128 db 0

buffer1       times 28 db 0
buffer2       times 28 db 0
buffer3       times 28 db 0
buffer4       times 28 db 0
buffer5       times 28 db 0

number_buffer db 10, 0

phonetic_spelling   times 128 db 0
