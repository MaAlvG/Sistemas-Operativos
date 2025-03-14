[BITS 16]
[ORG 0x7E00]

start:
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

exit:
    mov eax, 1
    xor ebx, ebx
    int 0x80

prompt db "Ingrese una letra (A-Z): ", 0x0D, 0x0A, 0x00

    
    phonetic_alphabet:
        db "Alpha", 0
        db "Bravo", 0
        db "Charlie", 0
        db "Delta", 0
        db "Echo", 0
        db "Foxtrot", 0
        db "Golf", 0
        db "Hotel", 0
        db "India", 0
        db "Juliett", 0
        db "Kilo", 0
        db "Lima", 0
        db "Mike", 0
        db "November", 0
        db "Oscar", 0
        db "Papa", 0
        db "Quebec", 0
        db "Romeo", 0
        db "Sierra", 0
        db "Tango", 0
        db "Uniform", 0
        db "Victor", 0
        db "Whiskey", 0
        db "X-ray", 0
        db "Yankee", 0
        db "Zulu", 0

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

times 512-($-$$) db 0  ; Rellenar hasta 512 bytes