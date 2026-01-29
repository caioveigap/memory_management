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

**Repo**: [Doug Lea's Malloc Repo](https://github.com/ennorehling/dlmalloc/tree/master?tab=readme-ov-file)

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

## 27/01/2026 - Investigando Free-List Allocators

