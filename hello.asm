BITS 16        

section   .data
    message:  db        "Hello, desde adentro", 10 

section   .text
    global _start

          
_start:   
    mov edx, message
    call print_string
    jmp exit                       ; invoke operating system to exit

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

dw 0xAA55 