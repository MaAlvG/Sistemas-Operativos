%include        './macros.asm'				;Incluye el archivo utilitario "macros.asm"

section .data						; Segmento de Datos inicializados
    ;variable para controlar que jugador escribe en cual turno 
    player db "1", 0
    p_size equ $-player
    
    win_flag db 0

    tieMessage db "Empate",0
    tMess_size equ $-tieMessage

    winPlayerX db "Jugador 1 ha ganado", 0
    wPX_size equ $- winPlayerX

    winPlayerO db "Jugador 2 ha ganado", 0
    wPO_size equ $-winPlayerO
    
    instructions db "Para ingresar las coordenadas, debe hacerlo en formato x,y no olvide poner la ','",10
                 db "    Jugador 1: o", 10
                 db "    Jugador 2: x", 10, 10
                 db "Tecla esc + enter: menu inicial", 10, 10, 0
    intruct_size equ $-instructions


    initial_message db "    Bienvenido al juego del gato",10
                    db "    Seleccione una opcion",10
                    db "    1. Iniciar juego",10
                    db "    2. Salir", 10, 0
    iM_size equ $- initial_message

    game_board db "   0   1   2 ", 10 
			   db "0 [_]|[_]|[_]", 10
               db "  ===========", 10
               db "1 [_]|[_]|[_]", 10
               db "  ===========", 10
               db "2 [_]|[_]|[_]", 10, 0
    gd_size equ $-game_board
	

	; --Constantes ligadas al Kernel--
        	sys_exit        EQU 	1
        	sys_read        EQU 	3
        	sys_write       EQU 	4
        	sys_open        EQU     5
        	sys_close       EQU     6
        	sys_execve      EQU     11
        	stdin           EQU 	0
        	stdout          EQU 	1
        	
section .bss						; Segmento de Datos no inicializados
	entrada 		resb 8			; Reserva espacio para 8 bytes
	
section .text
	global _start
	
_start:

copy_word:
    cld               ; Asegurar incremento hacia adelante
.copy_loop:
    lodsb             ; Cargar byte desde DS:SI en AL y avanzar SI
    cmp al, 0
    je .done_copy
    stosb             ; Almacenar AL en ES:DI y avanzar DI
    jmp .copy_loop
.done_copy:
    ret

copy_word_B
    mov al, [bx]
    cmp al, 0
    je .done_copy_B
    mov [di], al
    inc di
    inc bx
    jmp copy_word_B
done_copy_B
    xor al,al
    ret


;pantalla inicial, donde escribe 
initial_screen:
    call clean_board
    imprimeEnPantalla initial_message, iM_size
    xor edx, edx

    leeTeclado
    mov edx, entrada
    mov al, byte[edx]
    sub eax, '0'
    
    cmp rax, 1
    je loop_game_aux

    cmp rax, 2
    je FIN

    jmp initial_screen


loop_game_aux:
    
    imprimeEnPantalla instructions,intruct_size
    imprimeEnPantalla game_board,gd_size


loop_game:
    

	leeTeclado
	
        xor eax, eax
    	xor ebx, ebx
    	xor ecx, ecx
    	xor edx, edx

    	mov edx, entrada

   	    mov al, byte[edx] ; line
   	    inc edx
   	    inc edx
    	mov bl, byte[edx] ; column


	;Tony, si se cae el programa, borre estas dos lineas
	cmp rax, 27
	je initial_screen

    	sub eax, '0'
    	sub ebx, '0'
    	


; hace una conversion con las coordenadas ingresadas 0,1
; la ecuacion usada es 3a+b donde a es la primer coordenada y b la segunda
; de esto obtiene un numero con el cual se dirige a las posiciones especificas del string del tablero

    	mov ecx, 3
    	mul ecx
    	add al,bl
	

	cmp rax,0
	je coord1

	cmp rax,1
	je coord2

	cmp rax,2
	je coord3

	cmp rax,3
	je coord4

	cmp rax,4
	je coord5

	cmp rax,5
	je coord6

	cmp rax,6
	je coord7

	cmp rax,7
	je coord8

	cmp rax,8
	je coord9
    
    jmp loop_game

    coord1:
        mov rax, 17
        jmp update_board        

    coord2:
        mov rax, 21
        jmp update_board

    coord3:
        mov rax, 25
        jmp update_board

    coord4:
        mov rax, 45
        jmp update_board

    coord5:
        mov rax, 49
        jmp update_board

    coord6:
        mov rax, 53
        jmp update_board

    coord7:
        mov rax, 73
        jmp update_board

    coord8:
        mov rax, 77
        jmp update_board

    coord9:
        mov rax, 81
        jmp update_board



CreaLetras:
mov ax, [semilla]
mov cx, 48271
xor dx, dx
mul cx
add ax, 12345
adc dx, 0
mov [semilla], ax

;limpia el tablero de juego cada vez que sea necesario
clean_board:
    mov cl, "_"
    mov rax, 17
    lea rbx, [game_board + rax]
    mov [rbx], cl

    mov rax, 21
    lea rbx, [game_board + rax]
    mov [rbx], cl

    mov rax, 25
    lea rbx, [game_board + rax]
    mov [rbx], cl

    mov rax, 45
    lea rbx, [game_board + rax]
    mov [rbx], cl

    mov rax, 49
    lea rbx, [game_board + rax]
    mov [rbx], cl

    mov rax, 53
    lea rbx, [game_board + rax]
    mov [rbx], cl

    mov rax, 73
    lea rbx, [game_board + rax]
    mov [rbx], cl

    mov rax, 77
    lea rbx, [game_board + rax]
    mov [rbx], cl

    mov rax, 81
    lea rbx, [game_board + rax]
    mov [rbx], cl
ret

;actualiza el tablareo con la coordenada ingresada
update_board:
;revisa el string del tablero en la posicion indicada, si no es un "_" el campo ya esta ocupado, por lo que no cambia el turbno al siguiente jugador
;caso contrario compara el byte de player, y segun lo que este tenga almacenado decide si poner una "x" o una "o" 
	lea rbx, [game_board + rax]
	
	cmp byte[rbx], "_"
	jne flag_match

    mov rsi, player
    
    cmp byte[rsi], "0"
    je draw_x
    
    cmp byte[rsi], "1"
    je draw_o
    
    draw_x:
        mov cl, "x"
        jmp update
        
    draw_o:
        mov cl, "o"
        jmp update
        
    update:
        mov [rbx], cl
        call change_player
	    jmp flag_match
	

check:
    call check_line
    ret
;compara las lineas 3 posiciones de las lineas horizontales, si no encuentra 3 posiciones iguales compara las columnas  
check_line:

    mov rcx, 0
    
    check_line_loop:
        cmp rcx, 0
        je first_line
        
        cmp rcx, 1
        je second_line
    
        cmp rcx, 2
        je third_line
        
        call check_column
        ret
        
        first_line:
            mov rsi, 17
            jmp do_check_line
        
        second_line:
            mov rsi, 45
            jmp do_check_line
            
        third_line:
            mov rsi, 73 
            jmp do_check_line
            
        do_check_line:
            inc rcx
            
            lea rbx, [game_board + rsi]
            
            mov al, [ebx]
            cmp al, "_"
            je check_line_loop
            
            add rsi, 4
            lea rbx, [game_board + rsi]

            cmp al, [rbx]
            jne check_line_loop
            
            add rsi, 4
            lea rbx, [game_board + rsi]
            
            cmp al, [rbx]
            jne check_line_loop
            
        mov byte[win_flag], 1
        ret

;revisa las posiciones de las lineas verticales, si no encuentra 3 iguales revisa las diagonales
check_column:
    mov rcx, 0
    
    check_colum_loop:
        cmp rcx, 0
        je first_column
        
        cmp rcx, 1
        je second_column
        
        cmp rcx, 2
        je third_column
        
        call check_diagonal
        ret
        
        first_column:
            mov rsi, 17
            jmp do_check_column
            
        second_column:
            mov rsi, 21
            jmp do_check_column
            
        third_column:
            mov rsi, 25
            jmp do_check_column
            
        do_check_column:
            inc rcx
            
            lea rbx, [game_board + rsi]
            
            mov al, [rbx]
            cmp al, "_"
            je check_colum_loop
            
            add rsi, 18
            lea rbx, [game_board + rsi]
            
            cmp al, [rbx]
            jne check_colum_loop
            
            add rsi, 18
            lea rbx, [game_board + rsi]
            
            cmp al, [rbx]
            jne check_colum_loop
            
            mov byte[win_flag], 1
            ret
            
check_diagonal:
    mov rcx, 0
    
    check_diagonal_loop:
        cmp rcx, 0
        je first_diagonal
        
        cmp rcx, 1
        je second_diagonal
        
        ret
        
    first_diagonal:
        mov rsi, 17
        mov rdx, 32          
        jmp do_check_diagonal
        
    second_diagonal:
        mov rsi, 15
        mov rdx, 24
        jmp do_check_diagonal
        
    do_check_diagonal:
        inc rcx
        
        lea rbx, [game_board + rsi]
        
        mov al, [rbx]
        cmp al, "_"
        je check_diagonal_loop
        
        add rsi, rdx
        lea rbx, [game_board + rsi]
        
        cmp al, [rbx]
        jne check_diagonal_loop
        
        add rsi, rdx
        lea rbx, [game_board + rsi]
        
        cmp al, [rbx]
        jne check_diagonal_loop
        
    mov byte[win_flag], 1
    ret
    

flag_match:
    imprimeEnPantalla game_board,gd_size
    call check

    cmp byte[win_flag], 1
    je win_match

    mov eax, game_board
    iLoop:
        cmp byte[eax], "_"
        je loop_game

        cmp byte[eax], 0
        je tie_match
        
        inc eax
        jmp iLoop


change_player:
    
    xor byte[player], 1   
ret


tie_match:
    imprimeEnPantalla tieMessage, tMess_size
    jmp FIN


win_match:
    mov rsi, player
    
    cmp byte[rsi], "0"
    je winX
    
    cmp byte[rsi], "1"
    je winO

    winX:
    imprimeEnPantalla winPlayerX, wPX_size
    jmp FIN

    winO:
    imprimeEnPantalla winPlayerO, wPO_size
    jmp FIN


FIN:
    
	salir 
