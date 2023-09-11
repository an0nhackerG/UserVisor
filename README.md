# UserVisor
Uma simples demonstração de como o processo de Vmlaunch funciona

Esse projeto tem como objetivo demonstrar em modo de usuario como 
o processo de Vmlaunch funciona de maneira simplificada (muito simplificada mesmo)

O Processo de Vmlaunch aqui se consiste em somente escrever nos campos do VMCS falso 
os valores de HostRsp GuestRip GuestRSP, de maneira que seja demonstrado o processo 
de captura de contexto e retorno de contexto, assim como a criação de um stack para 
a hypervisor usar.

Quem sabe mais pra frente eu crie uma simulação bem mais realista de como isso funciona,
por agora é somente para mostrar esses pontos basicos.
