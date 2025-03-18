BITS 16
ORG 0x7E00          ; ORG 0 porque el código se carga en DS:0 = 0x7E00

start:
    
    ;=========imprimir instrucciones
    
    mov bx, newline_msg
    call print_string

    
    

    

    ; Imprimir nueva línea
    mov bx, newline_msg
    call print_string

    jmp game ; 
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
    mov bx, palabra_de_prueba
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
    mov al, [si]     ; char_buffer original
    mov bl, [di]     ; char_buffer ingresada
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
    call add_points               ; Sumar 2 puntos si fue correcto
    ret

show_score:
    mov bx, newline_msg
    call print_string
    call print_score
    jmp $  ; Mostrar puntaje final (puedes implementar esto)
    ret


;================================
generate_word:
    mov di, palabra_de_prueba

    call get_random_word
    mov bx, char_buffer
    call copy_word

    call get_random_word
    mov bx, char_buffer
    call copy_word

    call get_random_word
    mov bx, char_buffer
    call copy_word

    call get_random_word
    mov bx, char_buffer
    call copy_word

    call get_random_word
    mov bx, char_buffer
    call copy_word

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
;rutina para generar letras aleatorias
get_random_word:
    
.random_word_loop:
    call read_seed
    call get_letter         ; Llama a la función que genera un número aleatorio
    and al, 25              ; Limita el valor entre 0 y 25 (para letras a-z)
    add al, 'a'             ; Convierte el número en una letra ASCII
    mov [char_buffer], al   
    call save_seed      ; Guarda el resultado en la variable char_buffer
    ret

get_letter:
     ; Usa la parte baja del contador como nueva semilla

    mov ax, [seed]    ; Carga el valor de la semilla en AX
    mov cx, 48271        ; Constante multiplicativa
    xor dx, dx           ; Borra DX (DX:AX es un registro extendido de 32 bits)
    mul cx               ; Multiplica AX por CX (resultado en DX:AX)
    add ax, 16435        ; Suma una constante al resultado
    adc dx, 0            ; Agrega cualquier desbordamiento a DX
    mov [seed], ax
    ret    ; Guarda el nuevo valor en la semilla


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
;rutina para limpiar la pantalla

cls:
  pusha
  mov ah, 0x00
  mov al, 0x03  ; text mode 80x25 16 colours
  int 0x10
  popa
  ret

;================================
; Rutinas para imprimir la puntuacion 
print_score:
    mov bx, tens
    call print_string
    mov bx, units
    call print_string
    mov bx, newline_msg
    call print_string
    ret
;================================
; Rutinas para imprimir cadenas 

print_string:                                                                                                                                                                            
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
save_seed:
    mov ah, 0x03     ; Función de INT 13h para escribir en disco
    mov al, 1        ; Número de sectores a escribir (1 sector = 512 bytes)
    mov ch, 0        ; Cilindro 0
    mov cl, 5        ; Sector 5 (LBA 4 en CHS)
    mov dh, 0        ; Cabeza 0
    mov dl, 0x80     ; Disco duro (o 0x00 si es disquete)
    
    mov bx, seed  ; Dirección donde está la puntuación
    mov es, bx
    mov bx, 0x0000  

    int 0x13         ; Llamar a la BIOS para escribir el sector
    jc error_escritura ; Si hay error, mostrar mensaje
    ret              ; Retorna si la escritura es exitosa

error_escritura:
    mov si, msg_error_escritura
    call print_string
    ret

read_seed:
    mov ah, 0x02     ; Función de INT 13h para leer del disco
    mov al, 1        ; Número de sectores a leer (1 sector = 512 bytes)
    mov ch, 0        ; Cilindro 0
    mov cl, 5        ; Sector 5 (LBA 4 en CHS)
    mov dh, 0        ; Cabeza 0
    mov dl, 0x80     ; Disco duro (o 0x00 si es disquete)
    
    mov bx, seed  ; Dirección donde guardar la puntuación
    mov es, bx
    mov bx, 0x0000  

    int 0x13         ; Llamar a la BIOS para leer el sector
    jc error_lectura ; Si hay error, mostrar mensaje de error
    ret              ; Retorna si la lectura es exitosa

error_lectura:
    mov si, msg_error_lectura
    call print_string
    ret

;===============================
;Rutina para sumar numeros
;el numero debe estar iniciado como caracter

add_points:
    mov ax, [units] ;mover a ax el numero
    sub ax, '0'
    cmp ax, 9
    je add_tens
    add ax, 1
    add ax, '0'
    mov [units], ax ;devolver el numero a donde esta guardado
    ret

add_tens:
    sub ax, 9
    add ax, '0'
    mov [units], ax
    mov ax, [tens]
    sub ax, '0'
    cmp ax, 9
    je add_cents
    add ax, 1
    add ax, '0'
    mov [tens], ax
    ret

add_cents:
    mov bx, how
    call print_string
    ret

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

units db '0',0
tens db '0',0
how db "how?",0
palabra_de_prueba db "marco", 0

prompt db "Ingrese una letra (A-Z): ", 0x0D, 0x0A, 0x00

invalid_msg db "char_buffer invalida!", 0x0D, 0x0A, 0x00

user_spelling              times 128 db 0

buffer1       times 28 db 0
buffer2       times 28 db 0
buffer3       times 28 db 0
buffer4       times 28 db 0
buffer5       times 28 db 0

number_buffer db 10, 0

numberTest: dd 435

phonetic_spelling   times 128 db 0

char_buffer db "a", 0


seed equ 0x9000  ; Dirección donde se guardará la puntuación
msg_error_lectura db "Error al leer la puntuacion!", 0
msg_error_escritura db "Error al escribir la puntuacion!", 0
