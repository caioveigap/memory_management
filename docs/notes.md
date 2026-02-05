# DevLog - Alocador Dinâmico

## Recursos Úteis

**Artigo:** [Types of memory allocators](https://www.gingerbill.org/series/memory-allocation-strategies/)

**Artigo**: [Malloc Tutorial](https://danluu.com/malloc-tutorial/)

**Artigo**: [A memory allocator - Doug Lea](https://gee.cs.oswego.edu/dl/html/malloc.html)

**Artigo**: [Baby's First Garbage Collector](https://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/)

**Artigo**: [Dynamic Arrays in C](https://comp.anu.edu.au/courses/comp2310/labs/05-malloc/)

**Artigo**: [Untangling Lifetimes: The Arena Allocator](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator)

**Artigo**: [Dynamic Storage Allocation: A Survey and Critical Review](../../articles/dynamic%20memory%20allocation%20survey.pdf)

**Vídeo**: [Writing My Own Malloc - Tsoding](https://www.youtube.com/watch?v=sZ8GJ1TiMdk&t=57s)

**Vídeo**: [Coding Malloc in C from Scratch - Daniel Hirsch](https://www.youtube.com/watch?v=_HLAWI84aFA)

**Vídeo**: [Arenas in C](https://www.youtube.com/watch?v=3IAlJSIjvH0&t=92s)<br>
**-->** [Codebase com implementação de Arena Allocator](https://github.com/PixelRifts/c-codebase/tree/master)

**Repo**: [GLIBC Malloc Repo](https://github.com/sploitfun/lsploits/blob/master/glibc/malloc/malloc.c#L1680)

**Links de artigos do Akita**:
* [The 640K memory limit of MS-DOS](https://www.xtof.info/blog/?p=985)
* [The difference between booting MBR and GPT with GRUB](https://www.anchor.com.au/blog/2012/1...)
* [How Linux system boots up?](https://faun.pub/how-linux-system-boots-up-f15c9e0f7a96)
* [Rebasing Win32 DLLs](http://www.drdobbs.com/rebasing-win32...)
* [Commodore 64 memory map](http://www.awsm.de/mem64/)
* [Physical Address Extension - PAE Memory and Windows](https://docs.microsoft.com/en-us/prev...)
* [A visual guide to Go Memory Allocator from scratch (Golang)](https://blog.learngoprogramming.com/a...)
* [Understanding glibc malloc](https://sploitfun.wordpress.com/2015/02/10/understanding-glibc-malloc/)
* [Garbage collection in Python: things you need to know](http://rushter.com/blog/python-garbage-collector/)
* 

## 05/01/2026 - Pool Allocator de tamanho fixo (utilizando 'malloc')
Implementei o Pool Allocator utilizando a função 'malloc', fazendo com que os Chuncks sejam alocados dinâmicamente.
O Pool Allocator que estou implementando é bem simples e limitado, não permite definir<br> o tamanho dos chuncks em tempo
de execução. Em seguida farei o mesmo alocador porém alocando o buffer de uma só vez, e em seguida o dividindo em chuncks.
Isso permite que eu utilize uma syscall 'mmap'<br> para requisitar uma página ao SO, ou se for uma Pool pequena, pode se alocar o
chunck array diretamente na stack(apesar de não ser uma boa prática).<br>

## 05/01/2026 - Pool Allocator com chunck_size váriável ao inicializar uma Pool
A idéia desse tipo de Pool Allocator é pedir uma página de memória ao SO, preferencialmente utilizando `mmap`, e associar esta<br>
página à um `Chunck` *(Ex: 2KB por Chunck)*. Em seguida, o `Chunck` é dividido em `Blocos` em que cada bloco pode armenzar um total<br>
de **chunck_size** bytes.<br>
A system call `mmap` funciona pedindo memória ao SO, em forma de páginas, que normalmente são no mínimo 4096 bytes *(4KB)*. <br>
Quando o alocador precisa alocar um chunck, ele chamada mmap() *(que retorna um ponteiro para a página)* e então preenche aquele espaço<br>
de memória com o chunck. Porém ao alocar um novo chunck, não é necessário chamar pedir uma nova página ao SO caso ainda haja espaço restante <br>
na página previamente requisitada. Por exemplo:<br>
```
size_t block_size = 16;
size_t blocks_per_chunck = 64;
void *ptr = pool_alloc(&pool, block_size, blocks_per_chunck); // 1024 bytes allocated
                |
                * alloc_size = 1024
                * void *mem_ptr = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
                -> mem_ptr aponta para uma página de 4096 bytes, mesmo apenas pedindo 1024
Página:
|------------------|
|------------------| -> Alocados 1024 bytes
|P.................|
|..................|     
|..................|
|..................|
|..................|
|..................| -> Fim da página

* P é o ponteiro onde começa a região livre da página

Espaço livre: Offset 1024 até 4096
Note que foram alocados 1024 bytes, sobrando 3072 bytes
```
### Explicação sobre Pools:
Na implementação, a estrutura mais importante é a `Pool`. Pense na Pool como um contâiner, e nesse contâiner se armazenam blocos (de memória).<br> Porém os blocos 
ficam armazenados dentro de caixas, que seriam os `Chuncks`.<br> Dentro do "contâiner" é possível ter várias caixas de tamanhos variados, porém sempre seu tamanho será definido por: Capacidade de Blocos x Tamanho dos Blocos.<br> Os chuncks nada mais são que uma região de memória, que consegue armazenar
uma quantidade X de blocos.<br> O tamanho dos blocos também importa, portanto, o tamanho do bloco (bytes) define o tipo da `Pool` *(Contâiner)*. *Ex: Pool de 32 bytes*

## 06/01/2026 - Trabalhando na criação das Pools, e na arquiterua do alocador

### Arquitetura do alocador
Dividi meu Pool Allocator em dois "alocadores":<br>
* As Pools: são responsáveis por gerenciar os blocos de memória em si.
* O alocador geral: é responsável por gerenciar as páginas de memória e as pools.

`allocator_create()` cria a **root_page** e inicializa a estrutura `Allocator`. Também inicializa as `Pools` porém não aloca os chuncks para economizar memória("Lazy Initiation").<br>
O cabeçalho das Pools ficam na root_page. Se a root_page encher, basta alocar outra página (root_page 2) e unir os ponteiros em `Page_Allocator`. Por isso, acabei de perceber que será<br>
necessário colocar um Header do Page_Allocator no começo da root_page, antes de Allocator.<br>
Cada `Chunck` ocupará exatamente uma página (4096 bytes), por motivos de organização de memória e limpeza de 'lixo'. Os Chuncks devem ser alocados dinâmicamente, não há necessidade de pré-alocar Chuncks.<br>Blocos de menos de 8 bytes não são possíveis com arquitetura atual, pois o ponteiro **next** possui 8 bytes. Para permitir blocos com menos de 8 bytes seria necessário usar um offset de 4 bytes<br>
a partir do Chunck. 

A função `pool_create()` vai criar o Header de uma conforme o usuário necessita. O ponteiro `root_offset` deve ser usado para saber aonde por o Header da Pool a ser alocada.



## Elaborando arquitetura do alocador de memória

Um dos principios mais importantes de um alocador de memória é evitar fragmentação externa. Pensando nisto, o alocador que tenho em mente para implementar<br>
evita isso definindo regiões de memória em que serão alocados blocos menores, e regiões onde serão alocados blocos maiores.<br>

O alocador irá integrar 3 mecanismos de alocação:
* **Arenas**: A Arena é um setor da memória contínuo e reservado, que pode alocar blocos de qualquer tamanho de maneira sequencial.
* **Pool**: A Pool é uma estrutura que divide uma região da memória em blocos do mesmo tamanho, em que cada bloco livre guarda um ponteiro para o próximo.
* **Large Heap**: O Large Heap nada mais é que um alocador padrão, que usa uma Free-List para armazenar os blocos de memória que estão livres.<br>A diferêça desse mecanismo, é que são admitidos blocos de qualquer tamanho.


### Camadas do alocador:
* **[Camada 0] Gerente de Páginas do Backend**: O Alocador deve funcionar como um gerente, e os mecanismos de alocação são seus clientes.<br>Sempre que um sub-sistema de alocação (ex: Pool Allocator) precisar de memória, deve pedir ao backend. O gerente armazena informações de quais páginas estão livres, <br>quais estão ocupadas, e qual sub-sistema está alocando a página.

...


### Gerenciador de Páginas
Requisitos do gerenciador:
* O gerenciador (backend) deve atender a pedidos de requisição de memória, cujo tamanho pode equivaler a qualquer quantidade de páginas.
* Páginas devem ser unidas em setores maiores de potência de 2 (ex: 4KB, 8KB, 16KB). Isso é necessário para facilitar alocação.

A idéia que pensei para a estrutura desse gerenciador de páginas é a seguinte:<br>
As páginas serão divididas em `bins` de ordens variadas. A bin de ordem 0 possui uma lista de páginas de 4K livres. <br>Já a bin de ordem 1 possui uma lista de grupos de 2 páginas contínuas.
Na prática isso aconteceria da seguinte forma: Se por exemplo, o alocador Arena pedisse 10KB de memória contínua, o `Page Manager` deve buscar 4 páginas contínuas de memória.<br>
Então, verifica a bin de ordem 2 (16KB), se não estiver vazia, puxa o primeiro ponteiro, que aponta para o começo da primeira página de uma sequência de 4 páginas livres.<br>
Essa implementação não é a mais eficiente em termo de desperdicio de memória, porém é simples e de boa performance de alocação.<br>
<br>

Para guardar os metadados das páginas, irei utilizar um array de descritores de páginas.<br>
Em vez de guardar os metadados em um header diretamente no começo da página, criaremos um array separado que descreve os metadados de cada página na memória.<br>
**Isso evita o seguinte problema**:<br> O usuário pede exatamente 4KB de memória, o que equivale a uma página. Porém, o header da página precisa ficar no começo de sua região de memória.<br>
Como não é possível alocar o header antes, com o risco de corromper a memória da página anterior, seria necessário alocar 8KB (2 páginas) para o usuário, desperdiçando<br>
praticamente 50% da memória.

Para alocações maiores que um limite especificado (geralmente 128KB), alocações são feitas diretamente com a chamada ao sistema: mmap().


### Mecanismos de alocação
Acima do gerentes de páginas do backend, haverão seus "clientes", que são os mecanismos de memória: Arena, Pool, e Large-Heap.<br>
Cada um deles irá requisitar memória para o gerente, e irão administrar essa memória conforme suas políticas.<br>
**As requisições que cada mecanismo fazer devem ser extritamente em múltiplos das páginas (1, 2, 4, 8... páginas).**

#### <u>Large Heap</u>:
O Heap Allocator tem o principal objetivo de cuidar de alocações grandes, maiores que 512 bytes, apesar de não ser exatamente proíbido alocar objetos menores.<br>
Como as alocações dos 3 mecanismos de alocação ficarão conjuntas em memória, é uma boa prática tentar deixar as alocações do Large-Heap juntas em memória, pois<br>
ele aloca objetos de forma contínua. Por este motivo, o alocador Large-Heap gerencia sua memória em regiões, chamadas `Chuncks`.<br>
Toda vez que o Large-Heap pedir memória ao backend, irá alocar em chuncks, de no mínimo 32KB (8 páginas). Isso fará com que os objetos alocados pelo Heap-Allocator<br> 
fiquem em memória contínua, o que aumenta a performance devido a localidade.<br>
O maior chunck que o Large-Heap pode requisitar ao backend é de 128KB, acima disso, a alocação requisitada será atendida diretamente pelo SO (mmaped).

Os chunks devem guardar informações sobre a quantidade de memória utilizada, o ponteiro para a região de memória, e a capacidade total. <br>
Se um chunck ficar livre, pode ser devolvido ao backend.

#### <u>Pool</u>:
A Pool servirá para alocações pequenas, de no máximo 512 bytes. A Pool divide uma região de memória em blocos de tamanho `block_size`.<br>
Cada Pool possui regiões de memória que por sua vez possuem os blocos livres, cada região de memória é de tamanho **PAGE_SIZE**. Assim, a Pool guarda<br>
uma lista das regiões que ela gerencia, podendo aumentar dinâmicamente.

A Pool deverá pedir memória ao backend, e admnistrar conforme seu mecanismo. No caso, a Pool deve guardar uma lista de chuncks, como na implementação<br>
do protótipo. A diferença é que agora, os chuncks podem ser maiores que **PAGE_SIZE**, reduzindo o número de chuncks necessários. O tamanho das Pools<br>
seguirá o padrão das Bins do backend, em ordens de potência de 2. O tamanho mínimo do Chunck será 4KB, e o máximo 128KB. <br>
Além disso, a lista de blocos livres `Pool_Block *free_list` não será armazenada mais na Pool, e sim nos chuncks. <br>
Isso melhora a localidade de memória, e permite que quando um chunck ficar vazio, seja devolvido ao backend.

Outro ponto importante na refatoração é o fluxo de liberação da memória (a função `void pool_free(void *ptr)`). Como agora os chuncks não possuem 4KB unicamente,<br>
não é possível arredondar o ponteiro para o header do chunck que é "dono" do ponteiro a ser liberado. Para resolver este problema, iremos utilizar os descritores de página.<br>

**Isso funcionará da seguinte forma**:<br>
Na estrutura do descritor de página (`Page_Descriptor`), haverá um ponteiro genérico para o header da zona que ocupa aquelas páginas:<br>
```c
typedef struct Page_Descriptor {
    ...
    void *zone_header; // Ponteiro para o chunck "dono" do grupo de páginas
    ...
} Page_Descriptor;
```

Quando um ponteiro (void *ptr) precisa ser liberado, basta encontrar seu descritor chamando `Page_Descriptor *get_descriptor(void *ptr)`, <br>
e encontrar o chunck que o ponteiro pertence. Depois basta inserir o novo bloco livre na free_list do Chunck.

#### <u>Arena</u>:
A Arena é um mecanismo que aloca um espaço de memória contínua, e realiza alocações sequencialmente, sem possibilidade de liberar "blocos" memória.<br>
Para uma Arena ser realocada, deve verificar se ainda há espaço sobrando nas páginas que alocou. Isso porque ao criar uma Arena, o tamanho pedido<br>
pode não ser múltiplo **PAGE_SIZE**, fazendo com que o gerente backend aloque mais página do que o pedido. Ex: Se o usuário criar uma Arena de 10KB, <br>
o gerente irá alocar 16KB (4 páginas) para aquela Arena, portanto, sobrará espaço.<br>
Então, o usuário chamade arena_realloc(...), a Arena deve checar se há espaço sobrando primeiro, e se a memória a mais pedida é menor que essa sobra.<br>
Se for, não há necessidade de mover a Arena inteira para outra região da memória.