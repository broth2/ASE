# ASE - Projeto Final

O projeto está contido na pasta GPS_project.
Os source files estão na pasta main, tal como na estrutura de projetos apresentados nas aulas.
O webserver está na pasta main também no ficheiro app.py
O ficheiro utilizado no OTA update está na pasta build, é uma versão simples de um dos primeiros projetos usados nas aulas

GPS_project.c ->main file do projeto
utils.c -> funções chamadas no GPS_project.c
globals.c -> variáveis globais
pMod.c -> funções referentes ao módulo GPS
GPS_project.bin -> ficheiro a transferir por Wi-Fi para o ESP executar, conluindo o OTA update

A pasta Demo contém os vídeos utilizados na apresentação.

NOTA: as partições para os OTA updates têm 300KB porque o ficheiro que utilizamos para a demonstração tem apenas 180KB. Para correr uma versão atualizada do GPS Project, é necessário aumentar para 1M as partições, ou equivalente mediante as limitações do ESP


João Castanheira, Nuno Fahla
