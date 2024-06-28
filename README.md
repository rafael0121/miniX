# Sistema de Envio de Mensagens Curtas (Mini-X)

## Descrição do Projeto

Este projeto implementa um sistema de envio de mensagens curtas, similar a um mini-Twitter, utilizando um modelo multi-servidor. O objetivo é dominar a programação orientada a eventos utilizando a primitiva `select` e a temporização por sinais em C.

## Estrutura do Projeto

O projeto é composto por três programas:
1. **Servidor**: Responsável pelo controle da troca de mensagens.
2. **Cliente de Exibição**: Exibe as mensagens recebidas do servidor.
3. **Cliente de Envio**: Envia mensagens para o servidor.

## Protocolo de Comunicação

As mensagens são trocadas entre os clientes e o servidor utilizando o protocolo TCP. Cada mensagem tem o formato definido pela estrutura `msg_t`:

```c
typedef struct {
    unsigned short int type;
    unsigned short int orig_uid;
    unsigned short int dest_uid;
    unsigned short int text_len;
    unsigned char text[141];
} msg_t;
```

## Tipos de Mensagens

1. **OI (0)**: Mensagem de identificação do cliente.
2. **TCHAU (1)**: Mensagem de encerramento do cliente.
3. **MSG (2)**: Mensagem de texto.

## Compilação

Para compilar os programas, utilize o Makefile fornecido. No terminal, execute:

```c
make
```

## Execução

- **Para iniciar o servidor**:

```c
./server <port>
```
Substitua <port> pelo número da porta desejada (por exemplo, 12345).

- **Para iniciar o cliente de exibição**:

```c
./client_display <ID> <server_ip> <port>
```
Substitua <ID> pelo identificador único do cliente, <server_ip> pelo endereço IP do servidor (por exemplo, 127.0.0.1 para localhost), e <port> pelo número da porta usada pelo servidor.

- **Para iniciar o cliente de envio, execute**:

```c
./client_send <ID> <server_ip> <port>
```
Substitua <ID> pelo identificador único do cliente, <server_ip> pelo endereço IP do servidor (por exemplo, 127.0.0.1 para localhost), e <port> pelo número da porta usada pelo servidor.

## Exemplo de Uso

1. Compile todos os programas:
```c
make
```

2. Inicie o servidor em uma porta específica, por exemplo, 12345:
```c
./server 12345
```

3. Abra um novo terminal e inicie o cliente de exibição:
```c
./client_display 1 127.0.0.1 12345
```

4. Abra outro terminal e inicie o cliente de envio:
```c
./client_send 1001 127.0.0.1 12345
```

5. Envie uma mensagem a partir do cliente de envio. A mensagem será exibida no terminal do cliente de exibição.

## Detalhes de Implementação

### Servidor

O servidor aceita conexões de clientes, controla as mensagens de identificação (OI), encerramento (TCHAU), e mensagens de texto (MSG). O servidor utiliza a primitiva select para monitorar múltiplas conexões simultaneamente.

### Cliente de Exibição

O cliente de exibição se conecta ao servidor, envia uma mensagem de identificação (OI), e exibe todas as mensagens recebidas do servidor.

### Cliente de envio

O cliente de envio se conecta ao servidor, envia uma mensagem de identificação (OI), e permite ao usuário enviar mensagens de texto para outros clientes através do servidor.

### Tratamento de erros

O servidor e os clientes incluem mensagens de debug para auxiliar no diagnóstico de problemas. Mensagens de erro são impressas no console utilizando perror e printf.

## Limitações

- O número de clientes conectados é limitado a 10 clientes de exibição e 10 clientes de envio.
- As mensagens de texto têm um limite máximo de 140 caracteres.