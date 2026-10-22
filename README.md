# Integração WoT: JaCaMo, Node-RED, Mosquitto e PostgreSQL

Infraestrutura experimental para integração de Sistemas Multiagentes (MAS) com dispositivos IoT, utilizando JaCaMo, Jason, CArtAgO, Node-RED, Eclipse Mosquitto e PostgreSQL.

A implementação permite que um agente autônomo receba informações de propriedades, perceba notificações de eventos, solicite ações a dispositivos e acompanhe os estados reportados. As observações, os eventos e as solicitações de atuação são registrados em banco de dados, proporcionando uma base experimental para investigar a interação entre agentes autônomos e ambientes IoT/WoT.

> **Estado do projeto:** os cenários com `device_01`, `device_02` e `device_03` foram executados e testados. Um `device_04` foi utilizado para validar a recepção e a persistência de um parâmetro numérico. Consulte [Validação](#12-validação-e-limitações-conhecidas) para os limites dos testes.

## Sumário

- [1. Objetivo e escopo](#1-objetivo-e-escopo)
- [2. Arquitetura](#2-arquitetura)
- [3. Componentes](#3-componentes)
- [4. Dispositivos e cenários](#4-dispositivos-e-cenários)
- [5. Contratos de interação](#5-contratos-de-interação)
- [6. Tópicos MQTT](#6-tópicos-mqtt)
- [7. Persistência no PostgreSQL](#7-persistência-no-postgresql)
- [8. Estrutura do repositório](#8-estrutura-do-repositório)
- [9. Pré-requisitos e configuração](#9-pré-requisitos-e-configuração)
- [10. Executar e inspecionar a infraestrutura](#10-executar-e-inspecionar-a-infraestrutura)
- [11. Executar os clientes auxiliares](#11-executar-os-clientes-auxiliares)
- [12. Validação e limitações conhecidas](#12-validação-e-limitações-conhecidas)
- [13. Segurança e boas práticas](#13-segurança-e-boas-práticas)
- [14. Projeto de pesquisa](#14-projeto-de-pesquisa)

## 1. Objetivo e escopo

O projeto implementa uma infraestrutura experimental para estudar a interação entre Sistemas Multiagentes (MAS), dispositivos IoT e conceitos da Web of Things (WoT).

A infraestrutura permite:

1. encaminhar solicitações entre o agente Jason e os fluxos de integração;
2. receber valores de propriedades publicados por Things;
3. representar propriedades observadas como crenças do agente;
4. encaminhar notificações assíncronas de eventos;
5. encaminhar solicitações de atuação a dispositivos ou simuladores;
6. persistir observações, eventos e solicitações de atuação no PostgreSQL;
7. preservar tipos de dados em parâmetros JSON, incluindo números.

O projeto é uma **base experimental de integração**. Embora os tópicos usem uma convenção inspirada na WoT, a existência desses tópicos não significa que todos os dispositivos já estejam descritos por Thing Descriptions (TDs) completas, nem que exista descoberta automática de Things implementada.

### Escopo funcional atual

- Agente Jason `bob`, executado pela plataforma JaCaMo.
- Artefato CArtAgO `MQTTArt`.
- Node-RED como camada de integração entre REST/HTTP e MQTT.
- Eclipse Mosquitto como broker MQTT.
- PostgreSQL 17 para persistência.
- Três cenários principais de comunicação, observação e atuação (`device_01`, `device_02` e `device_03`).
- Um teste de persistência de parâmetro numérico (`device_04`).

## 2. Arquitetura

```text
                    SISTEMA MULTIAGENTE
                  ┌──────────────────────┐
                  │     Jason: bob       │
                  │  crenças e planos    │
                  └──────────┬───────────┘
                             │
                  ┌──────────▼───────────┐
                  │    JaCaMo/CArtAgO    │
                  │      MQTTArt         │
                  │   communication()    │
                  │   reporting()        │
                  │   monitoring()       │
                  │   acting()           │
                  └──────────┬───────────┘
                             │ REST/HTTP
                  ┌──────────▼───────────┐
                  │       Node-RED       │
                  │    roteamento,       │
                  │    normalização e    │
                  │    persistência      │
                  └────┬────────────┬────┘
                       │            │
                      MQTT          │ SQL
                       │            │
                ┌──────▼─────┐  ┌───▼─────────┐
                │  Mosquitto  │ │ PostgreSQL   │
                │ broker MQTT │ │ observações, │
                └──────┬──────┘ │ eventos e    │
                       │        │ ações        │
             ┌─────────▼──────┐ └──────────────┘
             │ Things e       │
             │ simuladores    │
             │ device_01..04  │
             └────────────────┘
```

### 2.1. Fluxo de observação de propriedade

1. Uma Thing publica uma propriedade em um tópico MQTT.
2. O Node-RED recebe e normaliza a mensagem.
3. A observação é registrada na tabela `observations`.
4. O Node-RED encaminha a informação ao artefato `MQTTArt` por REST/HTTP.
5. O agente Jason recebe a informação e atualiza a crença correspondente, conforme os planos de `bob.asl`.

```text
Thing → Mosquitto → Node-RED → PostgreSQL
                                └→ REST → MQTTArt → Jason
```

### 2.2. Fluxo de evento

1. A Thing publica uma notificação no tópico de evento.
2. O Node-RED registra o evento em `device_events`.
3. O Node-RED encaminha a notificação à operação `monitoring()` do artefato.
4. O artefato emite um sinal que pode ser percebido pelo agente.

```text
Thing → Mosquitto → Node-RED → PostgreSQL
                                └→ REST → MQTTArt → Jason
```

### 2.3. Fluxo de atuação

1. O agente solicita uma ação.
2. O artefato encaminha a solicitação ao endpoint `/act` do Node-RED por meio da infraestrutura REST.
3. A solicitação é registrada em `agent_actions`.
4. O Node-RED publica a mensagem no tópico MQTT da ação.
5. O dispositivo ou simulador processa a solicitação e publica o estado resultante.
6. O Node-RED persiste a propriedade reportada e atualiza o estado da solicitação conforme o mecanismo de confirmação atual.
7. O agente recebe o estado reportado como uma nova informação.

```text
Jason → MQTTArt → REST /act → Node-RED → PostgreSQL
                                      └→ Mosquitto → Thing
Thing → Mosquitto → Node-RED → Jason (estado reportado)
```

**Limitação conhecida:** a confirmação atual associa o estado reportado à solicitação pendente por correspondência entre dispositivo, ação, entrada e estado. A correlação explícita da resposta por `action_id` ainda não está implementada.

## 3. Componentes

| Componente | Tecnologia | Responsabilidade |
|---|---|---|
| Plataforma multiagente | JaCaMo | Executar e organizar o ambiente multiagente |
| Agente | Jason / AgentSpeak | Manter crenças e executar planos |
| Integração agente–ambiente | CArtAgO | Expor operações, propriedades observáveis e sinais |
| Artefato | `MQTTArt.java` | Encaminhar solicitações, propriedades e eventos |
| Integração de fluxos | Node-RED | Processar mensagens, realizar chamadas REST e publicar MQTT |
| Broker | Eclipse Mosquitto | Distribuir mensagens MQTT |
| Persistência | PostgreSQL 17 | Registrar observações, eventos e ações |
| Dispositivos e simuladores | ESP32 e Python | Publicar propriedades, receber comandos e gerar eventos |
| Orquestração | Docker Compose | Inicializar os serviços em contêineres |
| Modelo de descrição | WoT Thing Description (TD) | Referência para descrever propriedades, ações e eventos; integração completa com TDs é trabalho a desenvolver |

## 4. Dispositivos e cenários

### 4.1. `device_01` — iluminância e LED

| Tipo | Identificador |
|---|---|
| Propriedade | `illuminance` |
| Propriedade | `ledState` |
| Ação | `setLed` |
| Evento | `lowIlluminance` |
| Evento | `highIlluminance` |

Tópicos principais:

```text
wot/device_01/properties/illuminance
wot/device_01/properties/ledState
wot/device_01/actions/setLed
wot/device_01/events/lowIlluminance
wot/device_01/events/highIlluminance
```

Ações testadas:

```text
setLed(on)
setLed(off)
```

Os testes demonstraram a recepção dos comandos e a observação dos estados `on` e `off`. A presença de um ESP32 no projeto não deve ser interpretada como evidência de que todas as propriedades e eventos estejam associados a sensores físicos; a configuração física deve ser conferida no cenário usado.

### 4.2. `device_02` — temperatura, umidade e relé

| Tipo | Identificador |
|---|---|
| Propriedade | `temperature` |
| Propriedade | `humidity` |
| Propriedade | `relayState` |
| Ação | `setRelay` |
| Evento | `overheating` |

Tópicos principais:

```text
wot/device_02/properties/temperature
wot/device_02/properties/humidity
wot/device_02/properties/relayState
wot/device_02/actions/setRelay
wot/device_02/events/overheating
```

Ações testadas:

```text
setRelay(on)
setRelay(off)
```

O simulador Python gera valores de temperatura e umidade. Quando a temperatura ultrapassa o limiar configurado na implementação, é publicado o evento `overheating`.

> **Nota:** o estado do relé no cenário simulado não equivale ao acionamento comprovado de um relé físico.

### 4.3. `device_03` — motor

| Tipo | Identificador |
|---|---|
| Propriedade | `motorState` |
| Ação | `setMotor` |

Tópicos principais:

```text
wot/device_03/properties/motorState
wot/device_03/actions/setMotor
```

Ações testadas:

```text
setMotor(start)
setMotor(stop)
```

O recebimento dos estados `start` e `stop` foi validado por meio de mensagens MQTT enviadas com o aplicativo MQTT Dash. O teste validou o fluxo de comunicação e a representação do estado reportado pelo agente, não o acionamento de um motor físico.

> **Nota:** o estado do motor no cenário simulado não equivale ao acionamento físico. Caso seja utilizado um motor físico, sua montagem, acionamento e procedimentos de segurança devem ser seguidos adequadamente.

### 4.4. `device_04` — parâmetro numérico

O quarto cenário foi usado para testar a recepção e a persistência de um parâmetro numérico.

Exemplo de solicitação:

```text
action(device_04, setAirconditioner, 22.5)
```

Resultado observado no PostgreSQL:

| Campo | Valor |
|---|---|
| `thing_id` | `device_04` |
| `action_name` | `setAirconditioner` |
| `input` | `22.5` |
| `jsonb_typeof(input)` | `number` |
| `status` | `requested` |

Esse teste demonstra que o parâmetro numérico foi recebido e armazenado como número em JSONB. **Não demonstra a execução completa da ação** sobre um dispositivo.

## 5. Contratos de interação

O artefato `MQTTArt.java` disponibiliza quatro operações principais para integrar o agente aos fluxos.

### 5.1. `communication()`

Recebe uma solicitação destinada ao agente. O formato lógico da mensagem de entrada inclui um alvo, uma operação e dados.

Exemplo de payload JSON:

```json
{
  "target": "device_01",
  "operation": "setLed",
  "data": "on"
}
```

O Node-RED encaminha a mensagem para a operação `communication()` do artefato. A estrutura exata do payload deve permanecer consistente entre o cliente, o fluxo e o agente.

### 5.2. `reporting()`

Recebe o valor de uma propriedade reportada por uma Thing.

Exemplo lógico:

```text
report(device_02, temperature, 38.2)
```

O artefato converte valores JSON em termos Jason: números JSON são convertidos em termos numéricos, booleanos em átomos booleanos, strings em átomos e objetos/arrays em representação textual JSON. O agente possui planos para propriedades conhecidas, como `temperature`, `humidity`, `illuminance`, `ledState`, `relayState` e `motorState`.

### 5.3. `acting()`

Encaminha uma solicitação de atuação. Exemplo lógico:

```text
action(device_01, setLed, on)
```

A operação delega a solicitação ao mecanismo REST `act()`, que chega ao endpoint `POST /act` do Node-RED. O fluxo publica a ação no tópico correspondente:

```text
wot/<thing>/actions/<action>
```

O parâmetro pode ser textual ou numérico, conforme o contrato da ação. O tipo deve ser preservado durante a serialização e o processamento.

### 5.4. `monitoring()`

Recebe notificações assíncronas de eventos.

Exemplo lógico:

```text
event(device_02, overheating, 41.8)
```

O artefato emite o sinal `event`, que pode ser percebido pelo agente Jason.

## 6. Tópicos MQTT

A convenção de tópicos adotada é:

| Finalidade | Padrão |
|---|---|
| Mensagens destinadas ao agente | `wot/agents/bob` |
| Propriedades | `wot/{thing}/properties/{property}` |
| Eventos | `wot/{thing}/events/{event}` |
| Ações | `wot/{thing}/actions/{action}` |

Exemplos:

```text
wot/agents/bob

wot/device_01/properties/illuminance
wot/device_01/properties/ledState
wot/device_01/actions/setLed
wot/device_01/events/highIlluminance

wot/device_02/properties/temperature
wot/device_02/properties/humidity
wot/device_02/properties/relayState
wot/device_02/actions/setRelay
wot/device_02/events/overheating

wot/device_03/properties/motorState
wot/device_03/actions/setMotor
```

Os identificadores de Things, propriedades, ações e eventos devem coincidir entre dispositivos, fluxos Node-RED, agente e eventuais Thing Descriptions. A convenção de tópicos é uma convenção do projeto, não uma afirmação de que o formato seja imposto pela especificação WoT.

## 7. Persistência no PostgreSQL

A base de dados utilizada é `wotdb`, com usuário de aplicação `wotapp`. O esquema inicial está definido em:

```text
postgres/init/001_schema.sql
```

### 7.1. `observations`

Armazena observações de propriedades recebidas dos dispositivos.

| Campo | Descrição |
|---|---|
| `id` | Identificador da observação |
| `thing_id` | Identificador da Thing |
| `property_name` | Nome da propriedade |
| `numeric_value` | Valor numérico, quando aplicável |
| `text_value` | Valor textual, quando aplicável |
| `payload` | Payload em JSONB |
| `topic` | Tópico MQTT de origem |
| `observed_at` | Data e hora da observação |

### 7.2. `device_events`

Armazena eventos recebidos dos dispositivos.

| Campo | Descrição |
|---|---|
| `id` | Identificador do evento |
| `thing_id` | Identificador da Thing |
| `event_name` | Nome do evento |
| `payload` | Dados do evento em JSONB |
| `topic` | Tópico MQTT de origem |
| `received_at` | Data e hora de recepção |

### 7.3. `agent_actions`

Registra solicitações de atuação e o estado de processamento conhecido.

| Campo | Descrição |
|---|---|
| `id` | Identificador da solicitação no banco |
| `agent_id` | Agente solicitante |
| `thing_id` | Thing de destino |
| `action_name` | Nome da ação |
| `input` | Parâmetro em JSONB |
| `status` | Estado da solicitação |
| `requested_at` | Data e hora da solicitação |
| `completed_at` | Data e hora de conclusão |
| `result` | Resultado registrado em JSONB |

Estados documentados na implementação:

- `requested`: solicitação registrada e aguardando confirmação;
- `confirmed`: solicitação confirmada pelo mecanismo atual;
- `failed`: solicitação registrada como falha;
- `unknown`: resultado desconhecido.

A confirmação atual se baseia na correspondência entre o estado reportado e a solicitação pendente. A correlação explícita por `action_id` não está implementada nesta versão.

### 7.4. Consultas úteis

Últimas observações:

```sql
SELECT
    thing_id,
    property_name,
    numeric_value,
    text_value,
    payload,
    observed_at
FROM observations
ORDER BY observed_at DESC
LIMIT 20;
```

Eventos recentes:

```sql
SELECT
    thing_id,
    event_name,
    payload,
    received_at
FROM device_events
ORDER BY received_at DESC
LIMIT 20;
```

Últimas solicitações de atuação:

```sql
SELECT
    id,
    agent_id,
    thing_id,
    action_name,
    input,
    status,
    requested_at,
    completed_at,
    result
FROM agent_actions
ORDER BY requested_at DESC
LIMIT 20;
```

Verificar o tipo JSON dos parâmetros:

```sql
SELECT
    id,
    thing_id,
    action_name,
    input,
    jsonb_typeof(input) AS input_type,
    status
FROM agent_actions
ORDER BY id DESC
LIMIT 20;
```

## 8. Estrutura do repositório

A estrutura principal esperada é semelhante à seguinte. A estrutura exata deve ser confirmada nos arquivos versionados do repositório:

```text
project/
├── docker-compose.yml
├── .env                       # local; não versionar credenciais
├── .gitignore
├── README.md
├── AppDevice.py
├── AppMsgBob.py
├── app_jacamo/
│   ├── bob.asl
│   ├── main.xml                # configuração usada pelo projeto, se aplicável
│   └── src/
│       └── env/
│           └── net/
│               └── MQTTArt.java
├── node-red/
│   ├── package.json
│   ├── package-lock.json
│   └── flows/
│       └── flows.json
├── postgres/
│   └── init/
│       └── 001_schema.sql
└── sketchESP/
    └── sketchESP.ino
```

A configuração de execução do Node-RED, os arquivos do Mosquitto e outros recursos podem estar presentes conforme a versão do projeto. Diretórios de dependências instaladas, caches, credenciais locais e classes compiladas não devem ser tratados como código-fonte.

## 9. Pré-requisitos e configuração

### 9.1. Pré-requisitos

- Docker Engine;
- Docker Compose (plugin `docker compose`);
- acesso ao repositório;
- Python 3, caso os simuladores ou clientes auxiliares sejam utilizados;
- Arduino IDE ou ferramenta equivalente, caso seja utilizado um ESP32 físico.

Os serviços principais são executados em contêineres. Os requisitos de portas e variáveis dependem do `docker-compose.yml` versionado.

### 9.2. Variáveis de ambiente

O Compose utiliza `POSTGRES_PASSWORD` para configurar a senha do PostgreSQL. Se a configuração do projeto ainda não tiver um arquivo `.env`, crie-o na raiz:

```dotenv
POSTGRES_PASSWORD=defina_uma_senha_local
```

Substitua o exemplo por uma senha local apropriada. O arquivo `.env` deve permanecer fora do Git se contiver credenciais reais. Se o projeto precisar de outras variáveis, confira o `docker-compose.yml` e documente-as aqui.

### 9.3. Broker MQTT

Antes de executar os clientes, verifique a configuração do Mosquitto:

- porta do broker;
- autenticação exigida;
- usuários e senhas configurados;
- permissões de publicação e assinatura;
- endereço acessível para cada cliente.

Dentro da rede Docker, os serviços devem usar os nomes de serviço definidos no Compose. Um cliente executado fora da rede Docker deve usar o endereço do host ou outro endereço alcançável. Um ESP32 conectado à rede Wi-Fi, por exemplo, normalmente não resolve o nome interno `mosquitto` da rede Docker.

### 9.4. Clientes Python

Confira nos scripts auxiliares os parâmetros de conexão e configure o endereço e as credenciais adequados ao ambiente. Não publique senhas, tokens ou configurações privadas no repositório.

## 10. Executar e inspecionar a infraestrutura

Execute os comandos a partir da raiz do repositório, onde está o `docker-compose.yml`.

### 10.1. Validar a configuração e iniciar

```bash
docker compose config
docker compose up -d
```

O primeiro comando verifica a configuração resolvida do Compose. Se houver erros de variáveis ou YAML, corrija-os antes de iniciar.

### 10.2. Verificar os serviços

```bash
docker compose ps
```

### 10.3. Consultar os logs

```bash
docker compose logs --tail=100 mas
docker compose logs --tail=100 nodered
docker compose logs --tail=100 mosquitto
docker compose logs --tail=100 postgres
```

Para acompanhar continuamente um serviço, acrescente `-f`, por exemplo:

```bash
docker compose logs -f nodered
```

Os nomes `mas`, `nodered`, `mosquitto` e `postgres` precisam corresponder aos nomes dos serviços definidos no arquivo Compose. Se forem diferentes, utilize os nomes efetivamente declarados.

### 10.4. Portas

As portas publicadas documentadas na configuração atual incluem:

| Serviço | Porta no host | Uso |
|---|---:|---|
| Node-RED | `1880` | Editor e runtime dos fluxos |
| JaCaMo REST | `8080` | Operações REST da plataforma |
| Endpoint Jason | `3272` | Endpoint configurado pela imagem/serviço |
| Mosquitto | `1883` | Broker MQTT |

A disponibilidade depende do `docker-compose.yml`, da configuração do host e de não haver conflitos de portas. Confirme os mapeamentos com `docker compose ps` antes de acessar os serviços.

### 10.5. Encerrar os serviços

Para parar e remover os contêineres da aplicação, preservando os volumes:

```bash
docker compose down
```

> Não acrescente `-v` se quiser preservar os volumes persistentes, incluindo os dados do PostgreSQL e do Mosquitto.

## 11. Executar os clientes auxiliares

Os scripts da raiz podem ser utilizados em terminais separados, conforme o cenário experimental.

Cliente/simulador de dispositivo:

```bash
python3 AppDevice.py
```

Cliente para enviar mensagens destinadas ao agente:

```bash
python3 AppMsgBob.py
```

Confira o código dos scripts para verificar dependências Python, parâmetros de conexão e formato de entrada. Caso haja dependências listadas em um arquivo próprio, instale-as no ambiente virtual recomendado pelo projeto.

O formato lógico utilizado pelo cliente de mensagens inclui alvo, operação e dados. Exemplos de comandos dos cenários validados:

```text
device_01 setLed on
device_01 setLed off
device_02 setRelay on
device_02 setRelay off
device_03 setMotor start
device_03 setMotor stop
```

A ação `setAirconditioner` com parâmetro `22.5` foi usada em um teste separado de recepção e persistência. Não deve ser considerada uma ação totalmente integrada enquanto o fluxo de execução correspondente não for implementado e validado.

## 12. Validação e limitações conhecidas

### 12.1. Cenários testados

| Teste | Resultado registrado |
|---|---|
| Inicialização do agente Bob | Sucesso |
| Recepção de `illuminance` | Testada |
| Recepção de `temperature` e `humidity` | Testada |
| Recepção de `ledState` | Testada |
| Recepção de `relayState` | Testada |
| Recepção de `motorState` | Testada |
| Recepção dos eventos `lowIlluminance` e `highIlluminance` | Testada |
| Recepção do evento `overheating` | Testada |
| Solicitações `setLed(on/off)` | Testadas |
| Solicitações `setRelay(on/off)` | Testadas |
| Solicitações `setMotor(start/stop)` | Testadas |
| Persistência de observações | Testada |
| Persistência de eventos | Testada |
| Registro de solicitações de atuação | Testado |
| Preservação do parâmetro numérico `22.5` em JSONB | Testada separadamente |

### 12.2. Limitações conhecidas

1. **`device_04`:** o teste validou recepção e persistência do parâmetro numérico, não o ciclo completo de execução da ação.
2. **Confirmação de ações:** a correlação explícita por `action_id` não está implementada. O mecanismo atual associa uma solicitação pendente ao estado reportado por correspondência de campos.
3. **Dispositivos físicos e simulados:** o cenário de temperatura e umidade é simulado por Python. Os resultados de integração não comprovam, por si só, a existência de hardware físico em todos os cenários.
4. **Thing Descriptions:** a convenção de tópicos e os nomes de propriedades/ações/eventos preparam a integração conceitual com WoT, mas não significam que exista descoberta automática e interpretação completa de TDs.
5. **Conhecimento do agente:** nesta versão, os dados recebidos são tratados como informações do ambiente e representados como crenças Jason. O processo de construção, integração e evolução de conhecimento contextual não está implementado como parte desta infraestrutura.
6. **Cobertura de testes:** os resultados documentam os cenários executados. Não constituem garantia de tratamento de todas as falhas, condições de concorrência ou perda de conectividade.

## 13. Segurança e boas práticas

- Não versione `.env` nem arquivos com senhas, tokens ou credenciais.
- Mantenha `node-red/flows_cred.json` e configurações locais de usuários fora do repositório.
- Não exponha o editor Node-RED ou endpoints REST à Internet sem autenticação, controle de acesso e configuração de rede apropriados.
- Restrinja a publicação e a assinatura MQTT aos tópicos necessários.
- Use credenciais específicas para a aplicação em vez de credenciais administrativas.
- Não publique logs que contenham segredos ou dados privados.
- Faça cópias de segurança dos dados persistentes antes de mudanças de esquema.
- Revise `git status` e `git diff --cached` antes de cada commit.

## 14. Projeto de pesquisa

A infraestrutura dá suporte experimental à investigação da interação entre agentes autônomos e ambientes IoT/WoT.

Na versão atual:

- **Informação do ambiente:** valores de propriedades e notificações de eventos recebidos por MQTT.
- **Representação no agente:** informações processadas e representadas como crenças Jason.
- **Deliberação e atuação:** planos do agente podem solicitar ações por meio do artefato e dos fluxos.
- **Persistência experimental:** observações, eventos e solicitações de atuação são registrados no PostgreSQL.

---

## Licença e contribuições

A licença e as regras de contribuição devem ser definidas pelo mantenedor do repositório.
