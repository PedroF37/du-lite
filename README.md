# du-lite
## Projeto tosco de implementação simples e idiota do comando du no Linux em C
* Usa a biblioteca libutils que está em: `https://github.com/PedroF37/Utils`
* Foi compilado: `clang -Wall --Wextra --pedantic -std=c99 -lutils -o du-lite du-lite.c`
* É chamado: `$ ./du-lite <diretorio> <Número>; $ ./du-lite /home/pedro 10` onde 10 são os 10 diretórios mais gastadores de espaço em /home/pedro
* É simples (nada como o comando du). Pega apenas o tamanho dos diretórios base. Pega o diretório MEGA mas não o diretório MEGA/Estudo/Livros por exemplo, os subdiretórios são contabilizados para dar o tamanho total de MEGA
