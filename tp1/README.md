_Última alteração: 27/03, 9:18 pm, Caio Morais_

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
