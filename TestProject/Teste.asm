EXTERN RestoreContextMoreAssembly : PROC
.CODE
VmLauunch PROC
	mov rsp, rcx
	jmp rdx
VmLauunch ENDP


; Coloco os calores dos Gpr no stack
PushGpr MACRO
	push rax
	push rbx
	push rcx
ENDM
; Recupero os valores dos Gpr do stack
PopGpr MACRO
	pop rcx
	pop rbx
	pop rax
ENDM

; Lembre que a convenção do windows é a seguinte:
; Primeiro argumento: rcx
; Segundo argumento: rdx
; Terceiro argumento: r8
; Quarto argumento: r9
; E caso tenha mais eles vão na pilha (RSP)
RestoreContext2 PROC
	mov rax, [rcx] ; Coloco ProcessorContext.RAX em RAX
	mov rbx, [rcx + 8] ; Coloco ProcessorContext.RBX em RBX
	mov rcx, [rcx + 16] ; Coloco ProcessorContext.RCX em RCX
	; A partir daqui o valor de processor context
	; não está mais em RCX, somente o valor
	; do Registrador, porem não vamos usar mais
	; ProcessorContext, então tamo bem :)

	PushGpr ; Coloco os Gpr no stack
	mov rcx, rdx ; Coloco o HostRsp em rcx (primeiro argumento)
	mov rdx, rsp ; Coloco GuestRsp em rdx (Segundo argumento)
	mov r8, LaunchGuest ; Coloco GuestRIP em r8 (terceiro argumento)
	sub rsp, 20h ; Dou 32bits de folga para valores temporarios no stack
	call RestoreContextMoreAssembly ; Função no arquivo C
	; o RIP vai retornar nós para LaunchGuest

LaunchGuest:
	PopGpr ; Recupero os valores dos GPR na pilha
	add rbx, 100 ; faço uma demonstração somando o valor antigo de rbx em 100
	
	; Caso tudo de certo, os registradores devem estar com:
	; Rax original (100)
	; Rbx original (200) + 100
	; Rcx original (300)
RestoreContext2 ENDP
	
END