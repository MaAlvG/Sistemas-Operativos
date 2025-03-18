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
.spell_loop:
    call read_string
    mov bx, newline_msg
    call print_string

    ; Mostrar mensaje indicando lo ingresado
    mov bx, input_msg
    call print_string
    
    ; Imprimir lo ingresado (cadena almacenada en words_entered)
    mov bx, words_entered
    call print_string

    ; Imprimir nueva línea
    mov bx, newline_msg
    call print_string
    ;call cls

    call check_B
    jmp $
    ret
;================================
generate_word:
    ;=======================aqui se necesita generar la palabra
    ; Mostrar mensaje de solicitud

    mov bx, alpha       ; Apunta a "alpha", por ejemplo
    mov di, phonetic_spelling    ; Destino inicial
    call copy_word

    ; Insertar un espacio como separador
    ;mov byte [di], ' '

    ; Copiar la segunda palabra en el buffer
    mov bx, bravo        ; Apunta a "juliett", por ejemplo
    call copy_word

    ; Agregar un terminador nulo al final del buffer
    mov byte [di], 0 

    ; Mostrar el contenido del buffer
    mov bx, phonetic_spelling
    call print_string

    ret

copy_string:
    cld                  ; Asegurar que el avance sea hacia adelante
.loop:
    lodsb                ; Cargar byte de [SI] en AL y avanzar SI
    stosb                ; Almacenar AL en [DI] y avanzar DI
    test al, al          ; Verificar si es el terminador nulo (0)
    jnz .loop            ; Si no es 0, seguir copiando
    ret                  ; Termina la función

; Rutina: copy_word
; Copia una palabra terminada en 0 desde SI a DI.
; Al finalizar, DI apunta justo después del último carácter copiado.
copy_word:
    mov al, [bx]
    cmp al, 0
    je .done_copy
    mov [di], al
    inc di
    inc bx
    jmp copy_word
.done_copy:
    xor al, al
    ret




;================================
;rutina para comparar la palabra ingresada

checkB:
;mov di, phonetic_alphabet  ; Cargar dirección de inicio

next_wordB:
    mov bx, di
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

check_B:
    
    mov di, words_entered
    mov bl, [di]

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
    je letrak
    
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
letraA:
    mov si, alpha
    call print_string_si
    ret
letraB:
    mov si, bravo
    call print_string_si
    ret
letraC:
    mov si, charlie
    call print_string_si
    ret
letraD:
    mov si, delta
    call print_string_si
    ret
letraE:
    mov si, echo
    call print_string_si
    ret
letraF:
    mov si, foxtrot
    call print_string_si
    ret
letraG:
    mov si, golf
    call print_string_si
    ret
letraH:
    mov si, hotel
    call print_string_si
    ret
letraI:
    mov si, india
    call print_string_si
    ret
letraJ:
    mov si, juliett
    call print_string_si
    ret
letrak:
    mov si, kilo
    call print_string_si
    ret
letraL:
    mov si, lima
    call print_string_si
    ret
letraM:
    mov si, mike
    call print_string_si
    ret
letraN:
    mov si, november
    call print_string_si
    ret
letraO:
    mov si, oscar
    call print_string_si
    ret
letraP:
    mov si, papa
    call print_string_si
    ret
letraQ:
    mov si, quebec
    call print_string_si
    ret
letraR:
    mov si, rome
    call print_string_si
    ret
letraS:
    mov si, sierra
    call print_string_si
    ret
letraT:
    mov si, tango
    call print_string_si
    ret
letraU:
    mov si, uniform 
    call print_string_si
    ret
letraV:
    mov si, victor
    call print_string_si
    ret
letraW:
    mov si, whiskey
    call print_string_si
    ret
letraX:
    mov si, xray
    call print_string_si
    ret
letraY:
    mov si, yankee
    call print_string_si
    ret
letraZ:
    mov si, zulu
    call print_string_si
    ret
jmp $
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
    mov di, words_entered  ; DI apunta al words_entered
.read_loop:
    mov ah, 0       ; Espera una tecla (INT 16h)
    int 0x16
    cmp al, 0x0D   ; ¿Se presionó ENTER?
    je .done_read
        ; Eco: imprimir el carácter leído
    mov ah, 0x0E
    int 0x10
        ; Guardar el carácter en el words_entered
    mov [di], al
    inc di
    jmp .read_loop
.done_read:
    mov byte [di], 0   ; Terminar la cadena con 0
    xor al, al
    ret

read_string_B:
    mov di, words_entered  ; DI apunta al words_entered
.read_loop_B:
    mov ah, 0       ; Espera una tecla (INT 16h)
    int 0x16
    cmp al, 0x0D   ; ¿Se presionó ENTER?
    je .done_read_B
    ; Guardar el carácter en el words_entered
    mov [di], al
    inc di
    jmp .read_loop_B
.done_read_B:
    mov byte [di], 0   ; Terminar la cadena con 0
    ret

;===================================
; Mensajes y words_entered

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

words_entered       times 128 db 0

phonetic_spelling   times 128 db 0
