# mac0352

**Sequência recomendada de implementação**
1. servidor TCP básico (aceita conexão e eco simples)

**Caio Morais**

Eu comecei a fazer isso. Boa parte de implementar um cliente-servidor usando programação de soquetes é um grande ctrl-c, ctrl-v do livro do Stevens (aquele que o Daniel chegou a usar pro finalzinho de SO).

Escolhi a arquitetura de I/O multiplexado. Se a gente usar processos sem compartilhamento de memória, fica mais fácil de coordenar porquê, claro, não tem condição de corrida. Pág. 447

Recomendo que vocês leem o 6.1 e 6.2 do livro do Stevens pra entender como funciona isso. Os detalhes de implementação tão no capítulo 6 também, mas são bem chatos (coisas associadas ao pollfd). Usei o método poll.

Quando a gente se encontrar pessoalmente a gente conversa melhor sobre.

2. parser de comandos

**Caio Morais**

Isso eu já fiz, e já foi bem testado. É quase uma API, mas falta ainda documentar pra ficar mais claro como usar as funções.

3. CREATE + GET
4. SET
5. LIST
6. RESERVE + RELEASE
7. múltiplos clientes

**Caio Morais**

O servidor concorrente já está praticamente implementado, então essa parte também já está 80% ok.

8. locks
9. tratamento de desconexão

**Caio Morais**

Acho que dá pra dizer que esse já tá implementado.

10. logging

**Caio Morais**

O que está implementado está bem loggado. Mas tem que fazer isso pro que ainda vai ser escrito.

OBS.: o tcp_client.c não está pronto, o que eu fiz foi só passar pra uma IA fazer um cliente que funcione pra esse servidor, pra fazer testes, mas esse código aí possivelmente vai todo embora.
