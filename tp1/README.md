_Última alteração: 27/03, 9:18 pm, Caio Morais_

# Andamento do projeto

## Arquitetura usada

- Servidor utiliza I/O multiplexado
- Especificamente, usa a função poll para isso. Caso queiram entender melhor, recomendo o capítulo 6 do [livro do Stevens](https://github.com/ben-elbert/books/blob/master/UNIX%20Network%20Programming%2C%20Volume%201%2C%20Third%20Edition%2C%20The%20Sockets%20Networking%20API.pdf).
- Uma vez que a conexão é estabelecida, o servidor usa a função `handle_new_connection()`. Quando o kernel avisa que tem dados para enviar, o servidor chama `handle_client_data()`, que por sua vez usa as funções do parser para processar a mensagem do cliente.
- A interface do parser está bem documentada, mas acho difícil que mais alguma função além do `handle_client_data()` precise dela, então não preisa se preocupar. Caso queiram debuggar ela, usem o arquivo no diretório tests.

## Alguns TODOs

- Para quando forem implementar as operações:

Cada recurso possui:
- identificador único
- valor associado (string ou número)
- estado de disponibilidade (livre ou reservado)
- cliente responsável pela reserva (quando aplicável)

## Sequência recomendada de implementação

1. servidor TCP básico (aceita conexão e eco simples) = OK
2. parser de comandos = OK
3. CREATE + GET = falta fazer, implementar [aqui](https://github.com/aka-caim/mac0352/blob/73cec4b519a24266a7d5166ce2a374d3f8f9f90a/tp1/src/tcp_server.c#L155)
4. SET = falta fazer, implementar [aqui](https://github.com/aka-caim/mac0352/blob/73cec4b519a24266a7d5166ce2a374d3f8f9f90a/tp1/src/tcp_server.c#L183)
5. LIST = falta fazer, implementar [aqui](https://github.com/aka-caim/mac0352/blob/73cec4b519a24266a7d5166ce2a374d3f8f9f90a/tp1/src/tcp_server.c#L221)
6. RESERVE + RELEASE = falta fazer, implementar [aqui] (https://github.com/aka-caim/mac0352/blob/73cec4b519a24266a7d5166ce2a374d3f8f9f90a/tp1/src/tcp_server.c#L198)
7. múltiplos clientes = OK
8. locks = falta fazer. precisa das operações 3-6 pra poder ser implementado
9. tratamento de desconexão = falta fazer
10. logging = o que já foi implementado já está loggado. Falta revisar

# Tarefa prática 1. Programação com soquetes

## Arquivos

- `tcp_server.c / tcp_server.h`: implementação do servidor
- `tcp_client.c / tcp_client.h`: implementação do cliente
- `parser.c` / `parser.h`: parser de comandos, usado pelo servidor

## Build

No diretório src, compile os arquivos.

TODO: fazer um Makefile

```bash
gcc -o tcp_server tcp_server.c parser.c
gcc -o tcp_client tcp_client.c
```

## Uso

1. Em um terminal, inicie o servidor:

```bash
./tcp_server
```

2. Em outro(s) terminal(is), execute o cliente:

```bash
./tcp_client
```

3. Opcional: conecte a um IP/porta específico:

```bash
./tcp_client 127.0.0.1 6767
```

## Comandos permitidos

Envie um comando por linha, no cliente

- `CREATE <key>`
- `GET <key>`
- `SET <key> <value>`
- `RESERVE <resource>`
- `RELEASE <resource>`
- `LIST`

TODO: isso aqui já delimita um pouco como devemos especificar como são as mensagens no nosso protocolo. Vamos conversar isso, mas é algo bem trivial e arbitrário.

TODO: nesse momento, os comandos são parseados e processados. O servidor devolve um resultado legítimo, mas indicando que o comando ainda não foi implementado.

## Notas sobre a arquitetura usada

- O cliente é interativo e envia o conteúdo de stdin para o servidor
- Logs internos são enviados usando o syslog, tanto pelo cliente quanto pelo servidor.
- Todos os testes feitos foram usando distribuições Linux que usam systemd. Para verificar o journal do systemd em tempo real, execute o comando:

```bash
journalctl -f
```

- Em distribuições sem systemd (AntiX, Alpine, Artix, Devuan, Slackware, Void, etc.), é esperado que o seguinte comando imprima o log corretamente:

```bash
tail -f /var/log/syslog
```
