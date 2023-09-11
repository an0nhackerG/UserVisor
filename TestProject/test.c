#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <intrin.h>

/*
* Vers�o extremamente simplificada de como funciona o processo
* de vmlaunch de uma hypervisor. O objetivo � vermos como deve ser
* feita a captura do contexto (ou seja, capturar o estado dos processadores)
* e como o contexto deve ser restaurado ap�s o vmlaunch. Lembrando que � 
* uma representa��o muito simplificada, n�o escrevemos em nenhum dos campos
* importantes do VMCS, somente figurativamente no HostRsp GuestRip 
* GuestRsp. Porem acho que isso ajuda a demonstrar como a Hypervisor sabe
* onde ela deve entrar e como ela deve devolver o sistema operacional.
* 
* � importante deixar claro que eu estou restaurando o contexto de somente
* 1 processador (o processador 0), por mais que eu crie o contexto para 10.
* 
* o valor de HostRIP n�o � usado aqui, por que ele 
*/

typedef unsigned long long UINT64;
typedef unsigned long UINT32;

extern void VmLauunch(UINT64*, UINT64*);

#define TRUE 1
#define FALSE 0

typedef struct _CONTEXT CONTEXT, *PCONTEXT;
typedef struct _HYPERVISOR_STACK HYPERVISOR_STACK, *PHYPERVISOR_STACK;
typedef struct _PROCESSOR_CONTEXT PROCESSOR_CONTEXT, *PHV_PROCESSOR_CONTEXT;
typedef struct _HYPERVISOR_CONTEXT HYPERVISOR_CONTEXT, *PHV_HYPERVISOR_CONTEXT;


typedef struct _CONTEXT
{
	//Conjunto de registradores para simular um contexto bem pequeno
	UINT64 RAX;
	UINT64 RBX;
	UINT64 RCX;
	UINT64 RDX;
	UINT64 RSI;
}CONTEXT, *PCONTEXT;

extern void RestoreContext2(CONTEXT, UINT64*);

typedef struct _HYPERVISOR_STACK
{
	UINT64 Stack[24*1024]; // Stack disponivel para nossa hypervisor usar

	//Ponteiro para HypervisorContext, esse valor a hypervisor n�o pode mudar 
	PHV_HYPERVISOR_CONTEXT HypervisorContextStack;

}HYPERVISOR_STACK, *PHYPERVISOR_STACK;

typedef struct _PROCESSOR_CONTEXT
{
	UINT64 ProcessorNumber; // Numero do processador
	UINT64 ProcessorStatus; // Status do processador (figurativo)
	CONTEXT ProcessorContext; // Valores dos Registradores
	HYPERVISOR_STACK HypervisorStack; // Stack da hypervisor
}PROCESSOR_CONTEXT, * PHV_PROCESSOR_CONTEXT;

typedef struct _HYPERVISOR_CONTEXT
{
	UINT64 A; // Valor figurativo
	UINT64 B; // Valor figurativo
	UINT64 C; // Valor figurativo

	// Array de ponteiros para ProcessorContext (acessado pelo indice dos Vp)
	PHV_PROCESSOR_CONTEXT* ProcessorContextPointer; 
}HYPERVISOR_CONTEXT, *PHV_HYPERVISOR_CONTEXT;


/*
* Fun��o que vai ser apontada pelo GuestRIP.
* o final dessa fun��o deve conter um RtlRestoreContext ou similar,
* isso vai restaurar o contexto.
*/
int RestoreContextLessAssembly()
{
	PHV_PROCESSOR_CONTEXT Context;

	/*
	* Fa�o um calculo para conseguir chegar no stack da hypervisor,
	* assim conseguimos acessar os valores da main novamente sem precisar
	* passar como argumento pra essa fun��o (o que n�o � permitido). 
	* 
	* � importante ter esse valor j� que ele vai ser onde RtlCaptureContext
	* vai armazenar os valores (no nosso caso simplesmente alguns registradores)
	* e precisamos passar o ponteiro para esses valores para RtlRestoreContext
	* ou similar.
	*/
	Context = (PHV_PROCESSOR_CONTEXT)((UINT64)_AddressOfReturnAddress()
				  	- sizeof(HYPERVISOR_STACK) - 0x10);

	printf("Aqui vai RtlRestoreContext (ou sua fun��o que fa�a isso\n");
}

/*
* Essa � a primeira maneira e a que contem menos codigo assembly.
* em um codigo de hypervisor real ela contem apenas 20 linhas CASO 
* RtlRestoreContext n�o esteja disponivel, o que leva a gente a ter que criar
* nosso proprio RltRestoreContext. Caso RtlRestoreContext esteja disponivel
* essa fun��o n�o contem nenhum codigo assembly.
* 
* � importante deixar claro que VmLaunch em um cenario real, n�o recebe nenhum argumento.
* a Hypervisor pegaria o valor associado nos campos de Rsp e Rip e usa automaticamente,
* Como n�o � possivel executar a fun��o VmLaunch de forma real em um codigo a nivel
* de usuario, eu criei uma fun��o em assembly que faz algo """Parecido""", 
* VmLaunch nesse caso basicamente coloca o valor do Rsp criado por n�s no Rsp real
* e coloca o valor de Rip no Rip real, o que simula um Vmlaunch muito mas muito mas muito
* mas muito simples mesmo, porem � mais que essencial para fins demonstrativos.
*/
void RestoreContext1
(
	PHV_PROCESSOR_CONTEXT* ProcessorContextPointer,
	PHV_HYPERVISOR_CONTEXT HypervisorContext
)
{
	/*
	* Coloco hypervisorContext no topo do stack da hypervisor no processador 0.
	*/
	ProcessorContextPointer[0]->HypervisorStack.
	HypervisorContextStack = HypervisorContext;

	/*
	* Aqui tecnicamente eu estaria escrevendo em HostRip, eu basicamente falo
	* que quando acontecer uma VmExit o stack da nossa hypervisor vai ser esse.
	* e vale lembrar que no topo dele est� o famoso HypervisorContextStack.
	*/
	UINT64 Rsp = (UINT64)&ProcessorContextPointer[0]->
		HypervisorStack + sizeof(HYPERVISOR_STACK) - sizeof(CONTEXT);

	/*
	* E aqui o valor de GuestRIP que vai conter a fun��o que restaura o contexto
	* do processador, como eu disse, DEVE ser um ponteiro para a fun��o por que 
	* o campo GuestRIP n�o aceita uma fun��o com argumentos, ent�o passamos um 
	* ponteiro para o codigo que ele deve executar (j� que estamos escrevendo o RIP)
	*/
	UINT64 Rip = (UINT64)RestoreContextLessAssembly;
	/*
	* Meu Vmlaunch figurativo e extremamente simples. O codigo assembly dele �:
	* mov rsp, rcx
	* jmp rdx
	* e � isso :)
	*/
	VmLauunch(Rsp, Rip);
}

int RestoreContextMoreAssembly
(
	UINT64 HostRsp,
	UINT64* GuestRsp,
	UINT64* GuestRip
)
{
	/*
	* Associo GuestRsp em um outro HypervisorContext criado somente aqui
	* para demonstrar que conseguimos acessar os valores originais por meio
	* do stack da nossa hypervisor, o que � o esperado.
	* 
	* Ent�o se tudo ocorreu certo, os valores devem ser os mesmos que
	* os valores da fun��o main
	*/
	PHV_HYPERVISOR_CONTEXT HypervisorContext;
	HypervisorContext = (PHV_HYPERVISOR_CONTEXT)HostRsp;
	
	VmLauunch(GuestRsp, GuestRip);
}

int main()
{
	/*
	* Aloco HypervisorContext e verifico se a aloca��o ocorreu corretamente
	* verificando se o ponteiro � nulo.
	*/
	PHV_HYPERVISOR_CONTEXT HypervisorContext = malloc(sizeof(HYPERVISOR_CONTEXT));
	if (HypervisorContext == NULL) {
		return 1; // Falha na aloca��o de mem�ria
	}

	/*
	* Valores meramente ilustrativos somente para acessarmos depois se quisermos
	* porem eles n�o tem importancia nenhuma e est�o ai somente para demonstra��o
	*/
	HypervisorContext->A = 1;
	HypervisorContext->B = 2;
	HypervisorContext->C = 3;

	/*
	* Crio um array que consegue alocar o contexto para at� 10 Nucleos.
	* E verifico se o seu ponteiro � Nulo para ter certeza que tudo ocorreu
	* como o esperado.
	*/
	PHV_PROCESSOR_CONTEXT* ProcessorContextPointer = malloc(10 * 
		sizeof(PHV_PROCESSOR_CONTEXT));

	if (ProcessorContextPointer == NULL) {
		return 1; // Falha na aloca��o de mem�ria
	}

	/*
	* Associo o esses 10 Contextos criado no array de ponteiros
	* dentro de HypervisorContext no campo ProcessorContextPointer,
	* onde vai conter o ponteiro de todos os processadores.
	*/
	HypervisorContext->ProcessorContextPointer = ProcessorContextPointer;

	/*
	* E agora crio o contexto de cada um dos processadores
	*/
	for (int i = 0; i < 10; i++)
	{
		// Aloco o contexto do processador e verifico seu sucesso.
		PHV_PROCESSOR_CONTEXT ProcessorContext = malloc(sizeof(PROCESSOR_CONTEXT));
		if (ProcessorContext == NULL) {
			return 1; // Falha na aloca��o de mem�ria
		}

		/*
		* Associo um valor figurativo em ProcessorStatus
		* um valor no processorNumber para identificar o numero do processador
		* e coloco esse contexto no array de acordo com o indice que � baseado
		* no numero do processador.
		*/
		ProcessorContext->ProcessorStatus = TRUE;
		ProcessorContext->ProcessorNumber = i;
		HypervisorContext->ProcessorContextPointer[ProcessorContext->
											 ProcessorNumber] = ProcessorContext;

		/*
		* E ent�o eu fa�o uma captura figurativa do contexto do processador
		* atual. Aqui eu estou imaginando um cenario em que no momento que esse codigo
		* est� sendo rodado, o valor em RAX � 100 o RBX � 200 e RCX � 300, e isso deve
		* ser restaurado depois do vmlaunch para termos certeza que o sistema vai 
		* continuar rodando de onde ele parou. 
		* Nesse caso s� queremos ver que a hypervisor vai ser capaz de restaurar
		* os valores corretamente sem ter diretamente isso como argumento, j� que o valor
		* dos campos do vmcs n�o podem ser uma fun��o com argumento, ent�o precisamos dar
		* um jeito de armazenar esses valores no topo do stack da nossa hypervisor para que
		* consigamos acessar eles posteriormente. Existem 2 maneiras de fazermos isso, uma 
		* que contem mais codigo assembly e outra que contem menos (eu prefiro a com mais).
		* Porem ambas funcionam do seu proprio jeiton e ambas restauram esses valores
		* com sucesso para que o sistema volte com os valores corretos ap�s vmlaunch.
		*/
		ProcessorContext->ProcessorContext.RAX = 100;
		ProcessorContext->ProcessorContext.RBX = 200;
		ProcessorContext->ProcessorContext.RCX = 300;
	}
	
	/*
		Primeira maneira de realizar Vmlaunch e restaurar o contexto
		(descomente para usa-la e comente a outra maneira)
	*/	
	//RestoreContext1(ProcessorContextPointer[0], HypervisorContext);
	
	/*
	* Segunda e minha preferida maneira para restaurar o contexto,
	* Essa maneira tem mais codigo assembly, o que pode tornar ela
	* mais dificil de ler caso n�o tenha conhecimento, porem ela 
	* � mais clara no que faz, e por mais que tenha mais assembly
	* os calculos realizados para restaurar o contexto s�o mais 
	* claros. RestoreContext2 est� no arquivo assembly.
	* Comente e descomente a outra caso queira usar a outra
	*/
	ProcessorContextPointer[0]->HypervisorStack.
	HypervisorContextStack = HypervisorContext;

	UINT64* Rsp = ProcessorContextPointer[0]->
	HypervisorStack.HypervisorContextStack;

	RestoreContext2(ProcessorContextPointer[0]->ProcessorContext, Rsp);
	

	return 0;
}


