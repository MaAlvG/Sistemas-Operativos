BITS 16                  ; Definir el modo de operación en 16 bits
org 0x7c00   

section .data
    prompt db "Ingrese una letra (A-Z): ", 0

    
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
        db "Zulu", 0x00

    phonetic_pointers:
        dd phonetic_alphabet+0
        dd phonetic_alphabet+6
        dd phonetic_alphabet+12
        dd phonetic_alphabet+20
        dd phonetic_alphabet+26
        dd phonetic_alphabet+31
        dd phonetic_alphabet+39
        dd phonetic_alphabet+44
        dd phonetic_alphabet+50
        dd phonetic_alphabet+56
        dd phonetic_alphabet+64
        dd phonetic_alphabet+69
        dd phonetic_alphabet+74
        dd phonetic_alphabet+79
        dd phonetic_alphabet+88
        dd phonetic_alphabet+95
        dd phonetic_alphabet+101
        dd phonetic_alphabet+108
        dd phonetic_alphabet+115
        dd phonetic_alphabet+122
        dd phonetic_alphabet+128
        dd phonetic_alphabet+136
        dd phonetic_alphabet+143
        dd phonetic_alphabet+151
        dd phonetic_alphabet+157
        dd phonetic_alphabet+164

section .bss
    letter resb 1

section .text
    global _start

_start:
    ; Mostrar mensaje de entrada
    mov edx, phonetic_alphabet+12
    call print_string
    
    mov edx, prompt
    call print_string

    ; Salir
    jmp exit

; ==========================
; Subrutinas de impresión y lectura
; ==========================
print_string:
    mov ecx, edx
    mov edx, 100
    mov ebx, 1
    mov eax, 4
    int 0x80
    ret

read_char:
    mov eax, 3
    mov ebx, 0
    mov ecx, letter
    mov edx, 1
    int 0x80
    mov al, [letter]
    ret

exit:
    mov eax, 1
    xor ebx, ebx
    int 0x80
