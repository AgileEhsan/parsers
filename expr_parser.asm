;
; A recursive descent parser for expressions
;
; Expr    :: Term Expr_
; Expr_   :: + Term Expr_      { print '+' }
;         :: - Term Expr_      { print '-' }
;         :: EPSILON
; Term    :: Factor Term_
; Term_   :: * Factor Term_    { print '*' }
;         :: / Factor Term_    { print '/' }
;         :: EPSILON
; Factor  :: UFactor Factor_
; Factor_ :: ^ Factor          { print '^' }
;         :: EPSILON
; UFactor :: ID                { print id  }
;         :: ( Expr )
;

    global _start
    BITS   64

    SYS_READ        EQU        0x00
    SYS_WRITE       EQU        0x01
    SYS_FSTAT       EQU        0x05
    SYS_MMAP        EQU        0x09
    SYS_MUNMAP      EQU        0x0B
    SYS_EXIT        EQU        0x3C

    MAP_PRIVATE     EQU        0x02
    PROT_READ       EQU        0x01
    BUFFER_SIZE     EQU        4096

    END_OF_EXPR     EQU        0x0A
    WHITESPACE      EQU        ' '
    UNBALANCED_PARENTHESIS EQU 0x00
    INVALID_ID      EQU        0x01

[section .data]
    stdout_buffer   times 1024 dd 0
    stdout_offset   dd        0
    stdin_buffer    dq        0
    stdin_offset    dd        0
    stdin_size      dd        0
    op              db        0

    e_unbalanced    db    "Error : Expected ", 39, ')', 39, 10, 0
    e_invalid_id    db    "Error : Expected Letter or Digit", 10, 0
    errorMessage    dq    e_unbalanced, e_invalid_id

[section .text]

discard_spaces:
        mov eax, [stdin_offset]
        dec eax

.discard_loop:
        inc eax
        cmp eax, [stdin_size]
        jz .found_eof
        mov rbx, [stdin_buffer]
        cmp byte [rbx+rax], WHITESPACE
        jz .discard_loop

.found_eof:
        mov [stdin_offset], eax
        ret

expr:
        call term

.match_plus_or_minus:
        mov eax, [stdin_offset]
        cmp eax, [stdin_size]
        jz finish_matching
        call discard_spaces
        add rax, [stdin_buffer]
        mov al, [rax]
        cmp al, END_OF_EXPR
        jz finish_matching
        cmp al, '+'
        jz .match_next_term
        cmp al, '-'
        jnz finish_matching

.match_next_term:
        push rax
        inc dword [stdin_offset]
        call term
        pop rax
        call putchar
        jmp .match_plus_or_minus

finish_matching:
        ret


term:
        call factor

.match_star_or_div:
        mov eax, [stdin_offset]
        cmp eax, [stdin_size]
        jz finish_matching
        call discard_spaces
        add rax, [stdin_buffer]
        mov al, [rax]
        cmp al, '*'
        jz .match_next_factor
        cmp al, '/'
        jnz finish_matching

.match_next_factor:
        push rax
        inc dword [stdin_offset]
        call factor
        pop rax
        call putchar
        jmp .match_star_or_div


factor:
        call u_factor
        call factor_
        ret

factor_:
        mov eax, [stdin_offset]
        cmp eax, [stdin_size]
        call discard_spaces
        add rax, [stdin_buffer]
        cmp byte [rax], '^'
        jnz finish_matching
        inc dword [stdin_offset]
        call factor
        mov al, '^'
        call putchar
        ret

u_factor:
        call discard_spaces
        add rax, [stdin_buffer]
        mov al, [rax]
        cmp al, '('
        jz .match_bracketed_expr
        cmp al, END_OF_EXPR
        jz .proceed_to_new_expr

        mov r14d, INVALID_ID
        cmp al, '0'
        jc error
        cmp al, '9'
        jna .print_identifier
        cmp al, 'A'
        jc error
        cmp al, 'Z'
        jna .print_identifier
        cmp al, 'a'
        jc error
        cmp al, 'z'
        ja error

.print_identifier:
        call putchar

.proceed_to_new_expr:
        inc dword [stdin_offset]
        ret

.match_bracketed_expr:
        inc dword [stdin_offset]
        call expr
        mov eax, [stdin_offset]
        add rax, [stdin_buffer]
        cmp byte [rax], ')'
        mov r14d, UNBALANCED_PARENTHESIS
        jnz error
        inc dword [stdin_offset]
        ret

error:
        mov rax, [errorMessage+8*r14]
        call write_string
        ret


putchar:
        cmp dword [stdout_offset], BUFFER_SIZE
        jl ._putchar_
        call flush_buffer

._putchar_:
        mov edi, [stdout_offset]
        inc dword [stdout_offset]
        mov [rdi+stdout_buffer], al
        ret

write_string:
        mov rsi, rax

.loop_for_character:
        lodsb
        or al, al
        jz .finish_writing
        cmp dword [stdin_offset], BUFFER_SIZE
        jl ._loop_for_character_
        call flush_buffer

._loop_for_character_:
        mov edi, [stdout_offset]
        mov [rdi+stdout_buffer], al
        inc dword [stdout_offset]
        jmp .loop_for_character

.finish_writing:
        ret

; Flush Buffer to StdOut
flush_buffer:
        mov r13, rdi
        mov r15, rax
        mov r12, rsi
        mov edi, 1
        mov rsi, stdout_buffer
        mov edx, [stdout_offset]
        mov eax, SYS_WRITE
        syscall
        mov rsi, r12
        mov rax, r15
        mov rdi, r13
        mov dword [stdout_offset], 0
        ret

; Main routine
_start:
        xor edi, edi
        sub rsp, 144
        mov rsi, rsp
        mov eax, SYS_FSTAT
        syscall

        mov rax, [rsp+0x30]
        add rsp, 144
        mov [stdin_size], eax
        xor edi, edi
        mov esi, eax
        mov edx, PROT_READ
        mov r10d, MAP_PRIVATE
        mov r8d, edi
        mov r9d, edi
        mov eax, SYS_MMAP
        syscall

        mov [stdin_buffer], rax
        dec rax

.read_and_discard:
        inc rax
        cmp byte [rax], END_OF_EXPR
        jnz .read_and_discard

        inc rax
        sub rax, [stdin_buffer]
        mov [stdin_offset], eax

.parse_and_print:
        call expr
        mov al, 10
        call putchar
        mov eax, [stdin_offset]
        inc eax
        cmp eax, [stdin_size]
        jz .exit_now
        dec eax
        add rax, [stdin_buffer]
        cmp byte [rax], END_OF_EXPR
        jnz .exit_now
        inc dword [stdin_offset]
        jmp .parse_and_print

.exit_now:
        call flush_buffer
        xor edi, edi
        mov eax, SYS_EXIT
        syscall
