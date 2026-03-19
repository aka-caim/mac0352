# mac0352

**Sequência recomendada de implementação**
1. servidor TCP básico (aceita conexão e eco simples)

**Caio Morais**

Eu comecei a fazer isso. Boa parte de implementar um cliente-servidor usando programação de soquetes é um grande ctrl-c, ctrl-v do livro do Stevens (aquele que o Daniel chegou a usar pro finalzinho de SO).

Esse livro tem várias implementações canônicas de servidor-cliente (vejam as páginas 16-18).

Voltando pra especificação do EP, o servidor tem que:
1. atender múltiplos clientes simultaneamente
2. garantir consistência do estado compartilhado
3. evitar condições de corrida

E pode-se usar (pra atender múltiplos clientes):
1. threads
2. processos
3. I/O multiplexado

Pesquisei qual seria a mais fácil de implementar a concorrência no servidor. De longe, a mais complicada são as threads, porque a gente iria precisar usar um monte de mutex, lock e semáforo pra garantir os acessos consistentes, e evitar condição de corrida usando essas porras é chatíssimo, insuportável de debuggar.

Se a gente usar processos sem compartilhamento de memória, fica mais fácil de coordenar porquê, claro, não tem condição de corrida. Pág. 447

Se usarmos I/O multiplexado, com uma única thread, não existe condição de corrida, o estado compartilhado é acessado de maneira sequencial. Págs. 404 e 407

2. parser de comandos

**Caio Morais**

Isso eu já fiz, e já foi bem testado. É quase uma API, mas falta ainda documentar pra ficar mais claro como usar as funções.

3. CREATE + GET
4. SET
5. LIST
6. RESERVE + RELEASE
7. múltiplos clientes
8. locks
9. tratamento de desconexão
10. logging
