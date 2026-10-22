# Integração WoT, JaCaMo, Node-RED, Mosquitto e PostgreSQL

## 1. Visão geral

Este projeto implementa uma infraestrutura experimental para integração entre Sistemas Multiagentes (MAS), dispositivos IoT e serviços da Web das Coisas (Web of Things — WoT).

A arquitetura utiliza JaCaMo, Jason, CArtAgO, Node-RED, Eclipse Mosquitto e PostgreSQL para permitir que agentes autônomos recebam informações do ambiente, mantenham crenças sobre propriedades observadas, recebam notificações de eventos e solicitem ações a dispositivos.

A implementação constitui uma base experimental para pesquisas sobre agentes autônomos, descoberta de ambientes, affordances, descrições de Things e utilização de informações em ambientes IoT.

### Estado atual

A infraestrutura foi executada e testada com os seguintes componentes:

- Agente Jason `bob`, executado na plataforma JaCaMo.
- Artefato CArtAgO `MQTTArt`.
- Node-RED como mecanismo de integração entre REST/HTTP e MQTT.
- Eclipse Mosquitto como broker MQTT.
- PostgreSQL para persistência de observações, eventos e ações.
- Três dispositivos integrados aos fluxos de comunicação, observação e atuação.

Os cenários de atuação testados incluem:

- `device_01`: controle de LED, observação de iluminância e eventos de baixa e alta iluminância.
- `device_02`: simulação de temperatura e umidade, controle de relé e evento de superaquecimento.
- `device_03`: exemplo de atuação sobre um motor, com estados `start` e `stop`.

Um teste adicional com `device_04` demonstrou a recepção e a persistência de um parâmetro numérico (`22.5`) no PostgreSQL. A execução completa desse dispositivo ainda não faz parte dos cenários validados.

**Importante:** a validação da comunicação e dos estados reportados não significa, por si só, que todos os dispositivos correspondam a hardware físico. O dispositivo de temperatura e umidade é simulado por software, e a implementação física dos demais dispositivos deve ser verificada individualmente.

---

## 2. Objetivos

O projeto tem os seguintes objetivos técnicos:

1. Integrar um agente Jason a dispositivos e serviços MQTT.
2. Receber propriedades reportadas pelas Things e representá-las como crenças do agente.
3. Receber e processar notificações assíncronas de eventos.
4. Encaminhar solicitações de atuação do agente para dispositivos.
5. Persistir observações, eventos e solicitações de ações em um banco de dados relacional.
6. Preservar tipos de dados, incluindo valores numéricos utilizados como parâmetros de ações.
7. Preparar a infraestrutura para integração com Thing Descriptions (TDs) da WoT.
8. Fornecer uma base experimental para investigar mecanismos de descoberta, interpretação e utilização de informações por agentes autônomos.

---

## 3. Arquitetura

A arquitetura é composta por cinco serviços e uma camada de dispositivos.

```text
+-------------------------------------------------------+
|                  Sistema Multiagente                  |
|                                                       |
|                    Jason - Bob                        |
|                                                       |
|       Crenças, solicitações e planos de atuação       |
+---------------------------+---------------------------+
                            |
                            v
+-------------------------------------------------------+
|                   JaCaMo / CArtAgO                    |
|                                                       |
|                     MQTTArt                           |
|                                                       |
|    communication() | reporting() | monitoring()       |
|                       acting()                        |
+---------------------------+---------------------------+
                            |
                       REST / HTTP
                            |
                            v
+-------------------------------------------------------+
|                      Node-RED                         |
|                                                       |
|  Recepção de solicitações e processamento de ações    |
|  Normalização de propriedades e eventos               |
|  Persistência e confirmação de ações                  |
+---------------------------+---------------------------+
                            |
                            v
+-------------------------------------------------------+
|                   Mosquitto MQTT                      |
|                                                       |
|              Broker de mensagens MQTT                 |
+---------------------------+---------------------------+
                            |
                            v
+-------------------------------------------------------+
|                     Dispositivos                      |
|                                                       |
|  device_01: iluminância, LED e eventos                |
|  device_02: temperatura, umidade e relé               |
|  device_03: estado do motor e ações start/stop        |
|  device_04: cenário experimental de parâmetro numérico|
+-------------------------------------------------------+

                            |
                            v
+-------------------------------------------------------+
|                     PostgreSQL                        |
|                                                       |
|    observations | device_events | agent_actions       |
+-------------------------------------------------------+
```

O PostgreSQL é utilizado pelo Node-RED para armazenar os dados recebidos e registrar o ciclo das solicitações de atuação.

Os principais fluxos são:

### 3.1. Observações de propriedades

```text
Thing
  |
  v
MQTT
  |
  v
Node-RED
  |
  +--> Normalização
  |
  +--> PostgreSQL: observations
  |
  v
REST / reporting()
  |
  v
MQTTArt
  |
  v
Crenças do agente Jason
```

### 3.2. Notificações de eventos

```text
Thing
  |
  v
MQTT
  |
  v
Node-RED
  |
  +--> PostgreSQL: device_events
  |
  v
REST / monitoring()
  |
  v
MQTTArt
  |
  v
Evento percebido pelo agente
```

### 3.3. Solicitações de atuação

```text
Agente Jason
  |
  v
MQTTArt.acting()
  |
  v
JaCaMo REST
  |
  v
Node-RED: POST /act
  |
  v
PostgreSQL: agent_actions
  |
  v
Publicação MQTT
  |
  v
Dispositivo
  |
  v
Publicação do estado atualizado
  |
  v
Node-RED
  |
  +--> Atualização de agent_actions
  |
  +--> Persistência da propriedade
  |
  v
Agente Jason recebe o estado reportado
```

A confirmação atual das ações é baseada na correspondência entre a solicitação pendente e o estado reportado pela propriedade do dispositivo.

A correlação explícita das confirmações por `action_id` não está implementada nesta versão.

---

## 4. Componentes e tecnologias

| Componente | Tecnologia | Responsabilidade |
|---|---|---|
| Sistema Multiagente | JaCaMo | Executar e organizar o ambiente multiagente |
| Agente | Jason / AgentSpeak | Manter crenças e executar planos |
| Integração agente-ambiente | CArtAgO | Disponibilizar operações e observações |
| Artefato | `MQTTArt.java` | Encaminhar solicitações, propriedades e eventos |
| Integração de fluxos | Node-RED | Processar mensagens, realizar chamadas REST e publicar MQTT |
| Broker | Eclipse Mosquitto | Distribuir mensagens MQTT |
| Persistência | PostgreSQL 17 | Registrar observações, eventos e ações |
| Dispositivos e simuladores | ESP32 e Python | Publicar propriedades, receber comandos e gerar eventos |
| Orquestração | Docker Compose | Inicializar os serviços da infraestrutura |
| Descrição de Things | WoT Thing Description | Descrever propriedades, ações e eventos dos dispositivos |

---

## 5. Dispositivos integrados

### 5.1. device_01 — iluminância e LED

O dispositivo publica valores de iluminância, disponibiliza o estado do LED e emite eventos quando os valores observados atingem determinadas condições.

| Elemento | Nome |
|---|---|
| Propriedade | `illuminance` |
| Propriedade | `ledState` |
| Ação | `setLed` |
| Evento | `lowIlluminance` |
| Evento | `highIlluminance` |

Exemplos de tópicos:

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

Os testes de integração demonstraram a recepção dos comandos e a observação dos estados `on` e `off`.

### 5.2. device_02 — temperatura, umidade e relé

O dispositivo utiliza um simulador Python para gerar valores de temperatura e umidade, além de disponibilizar um estado de relé e um evento de superaquecimento.

| Elemento | Nome |
|---|---|
| Propriedade | `temperature` |
| Propriedade | `humidity` |
| Propriedade | `relayState` |
| Ação | `setRelay` |
| Evento | `overheating` |

Exemplos de tópicos:

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

O simulador publica valores de temperatura e umidade. Quando a temperatura ultrapassa o limiar definido na implementação, publica o evento `overheating`.

**Nota:** o comportamento do relé no simulador não deve ser interpretado como acionamento de um relé físico.

### 5.3. device_03 — motor

O terceiro dispositivo implementa um cenário de atuação com estados `start` e `stop`.

| Elemento | Nome |
|---|---|
| Propriedade | `motorState` |
| Ação | `setMotor` |

Tópicos:

```text
wot/device_03/properties/motorState
wot/device_03/actions/setMotor
```

Ações testadas:

```text
setMotor(start)
setMotor(stop)
```

O agente recebeu os estados reportados `start` e `stop` após as solicitações de atuação.

A implementação física do motor deve ser documentada separadamente caso seja utilizado hardware real.

### 5.4. device_04 — teste de parâmetro numérico

O quarto dispositivo foi utilizado em um teste de recepção e persistência de parâmetros numéricos.

Solicitação de exemplo:

```text
action(device_04, setAirconditioner, 22.5)
```

O Node-RED recebeu a estrutura de ação e o PostgreSQL registrou:

| Campo | Valor |
|---|---|
| `thing_id` | `device_04` |
| `action_name` | `setAirconditioner` |
| `input` | `22.5` |
| `jsonb_typeof(input)` | `number` |
| `status` | `requested` |

Esse teste confirma a preservação do tipo numérico na recepção da solicitação e na persistência do parâmetro.

Ele não demonstra ainda a execução completa de uma ação sobre um dispositivo físico ou simulado.

---

## 6. Contratos de interação do MQTTArt

O artefato `MQTTArt.java` disponibiliza quatro operações principais.

### 6.1. Communication

Recebe uma solicitação destinada ao agente.

Formato lógico:

```text
request(Target, Operation, Data)
```

Exemplo de mensagem MQTT:

```json
{
  "target": "device_01",
  "operation": "setLed",
  "data": "on"
}
```

O Node-RED encaminha os campos para a operação `communication()` do artefato.

### 6.2. Reporting

Recebe o valor de uma propriedade reportada por uma Thing.

Formato lógico:

```text
report(Thing, Property, Value)
```

Exemplo:

```text
report(device_02, temperature, 38.2)
```

Os valores numéricos são convertidos em termos numéricos Jason. Strings são representadas como átomos.

O agente utiliza planos específicos para propriedades conhecidas, como `temperature`, `humidity`, `illuminance`, `ledState`, `relayState` e `motorState`.

### 6.3. Acting

Encaminha uma solicitação de atuação.

Formato lógico:

```text
action(Thing, Action, Input)
```

Exemplo:

```text
action(device_01, setLed, on)
```

O artefato delega a execução ao mecanismo `act()` da classe-base REST. O Node-RED recebe a solicitação no endpoint:

```text
POST /act
```

A ação é publicada no tópico:

```text
wot/<thing>/actions/<action>
```

O parâmetro `Input` pode ser uma string ou um número, conforme o tipo de ação.

### 6.4. Monitoring

Recebe notificações assíncronas de eventos.

Formato lógico:

```text
event(Thing, Event, Data)
```

Exemplo:

```text
event(device_02, overheating, 41.8)
```

O artefato emite o sinal `event`, percebido pelo agente Jason.

---

## 7. Tópicos MQTT

A arquitetura utiliza os seguintes padrões de tópicos:

| Finalidade | Padrão |
|---|---|
| Comunicação destinada ao agente | `wot/agents/bob` |
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

Os dispositivos, fluxos Node-RED e eventuais Thing Descriptions devem utilizar os mesmos identificadores e formatos de payload.

---

## 8. Persistência no PostgreSQL

O banco de dados utilizado é `wotdb`, com o usuário de aplicação `wotapp`.

O esquema inicial é definido em:

```text
postgres/init/001_schema.sql
```

### 8.1. Tabela observations

Armazena observações de propriedades recebidas dos dispositivos.

Principais campos:

| Campo | Descrição |
|---|---|
| `id` | Identificador da observação |
| `thing_id` | Identificador da Thing |
| `property_name` | Nome da propriedade |
| `numeric_value` | Valor numérico, quando aplicável |
| `text_value` | Valor textual, quando aplicável |
| `payload` | Payload original em JSONB |
| `topic` | Tópico MQTT de origem |
| `observed_at` | Data e hora da observação |

### 8.2. Tabela device_events

Armazena eventos recebidos dos dispositivos.

| Campo | Descrição |
|---|---|
| `id` | Identificador do evento |
| `thing_id` | Identificador da Thing |
| `event_name` | Nome do evento |
| `payload` | Dados do evento em JSONB |
| `topic` | Tópico MQTT de origem |
| `received_at` | Data e hora de recepção |

### 8.3. Tabela agent_actions

Registra as solicitações de atuação dos agentes e seus estados de processamento.

| Campo | Descrição |
|---|---|
| `id` | Identificador da solicitação |
| `agent_id` | Agente solicitante |
| `thing_id` | Thing de destino |
| `action_name` | Nome da ação |
| `input` | Parâmetro de entrada em JSONB |
| `status` | Estado da solicitação |
| `requested_at` | Data e hora da solicitação |
| `completed_at` | Data e hora de conclusão |
| `result` | Resultado registrado em JSONB |

Estados utilizados:

- `requested`: solicitação registrada e aguardando confirmação.
- `confirmed`: solicitação confirmada pelo mecanismo atual.
- `failed`: solicitação registrada como falha.
- `unknown`: resultado desconhecido.

A confirmação atual utiliza a correspondência entre o estado reportado e a solicitação pendente. A correlação explícita por `action_id` ainda não está implementada.

### 8.4. Consultas úteis

Consultar as últimas observações:

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

Consultar os eventos recentes:

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

Consultar as últimas solicitações de atuação:

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

---

## 9. Estrutura do projeto

A estrutura principal da aplicação é organizada em diretórios para os serviços, o agente, os fluxos e os dispositivos.

```text
artigo/
├── docker-compose.yml
├── .env
├── .gitignore
├── README.md
│
├── app_jacamo/
│   ├── main.jcm
│   ├── bob.asl
│   └── src/
│       └── env/
│           └── net/
│               └── MQTTArt.java
│
├── node-red/
│   ├── settings.js
│   ├── package.json
│   └── [arquivos de fluxos e configuração]
│
├── mosquitto/
│   ├── config/
│   │   └── mosquitto.conf
│   ├── data/
│   └── log/
│
├── postgres/
│   └── init/
│       └── 001_schema.sql
│
├── sketchESP/
│   └── sketchESP.ino
│
├── AppDevice.py
└── AppMsgBob.py
```

Os nomes e caminhos acima devem ser conferidos com a versão efetivamente mantida no repositório.

Arquivos de execução, dependências instaladas, caches e ambientes virtuais não devem ser tratados como código-fonte da aplicação.

---

## 10. Pré-requisitos

- Docker Engine.
- Docker Compose.
- Acesso ao repositório do projeto.
- Python 3, caso sejam utilizados os simuladores ou clientes auxiliares.
- Arduino IDE ou ferramenta equivalente, caso seja utilizado o ESP32 físico.

Os serviços principais são executados em contêineres Docker.

---

## 11. Configuração

### 11.1. Variáveis de ambiente

O Docker Compose utiliza a variável `POSTGRES_PASSWORD` para configurar a senha do usuário PostgreSQL.

Crie o arquivo `.env` na raiz do projeto, se ainda não existir:

```dotenv
POSTGRES_PASSWORD=defina_uma_senha_local
```

Substitua o valor por uma senha apropriada para o ambiente.

Não versione o arquivo `.env` se ele contiver credenciais reais.

### 11.2. Broker MQTT

O Mosquitto deve estar configurado para aceitar conexões dos serviços autorizados e dos clientes de teste.

Confira:

- Porta MQTT.
- Usuário e senha, quando exigidos.
- Configuração de autenticação.
- Permissões de publicação e assinatura.
- Endereço acessível pelos clientes externos.

Dentro da rede Docker, os serviços devem utilizar os nomes definidos no Compose. Um cliente executado fora dessa rede deve utilizar o endereço do host ou o IP acessível do broker.

Não assuma que o hostname `mosquitto` é resolvido diretamente por um ESP32 conectado à rede Wi-Fi.

### 11.3. Clientes Python

Os scripts auxiliares devem utilizar o endereço e as credenciais do broker disponíveis no ambiente de execução.

Antes de executar os clientes, verifique os parâmetros de conexão no código ou na configuração local.

Não compartilhe arquivos contendo senhas ou endereços privados sem necessidade.

---

## 12. Executar a infraestrutura

Execute os comandos a partir do diretório que contém o arquivo `docker-compose.yml`.

### 12.1. Iniciar os serviços

```bash
docker compose up -d
```

### 12.2. Verificar o estado dos contêineres

```bash
docker compose ps
```

### 12.3. Consultar os logs

JaCaMo:

```bash
docker compose logs -f mas
```

Node-RED:

```bash
docker compose logs -f nodered
```

Mosquitto:

```bash
docker compose logs -f mosquitto
```

PostgreSQL:

```bash
docker compose logs -f postgres
```

Os nomes utilizados nos comandos devem corresponder aos nomes dos serviços declarados no arquivo `docker-compose.yml`.

### 12.4. Interfaces e portas

Na configuração atual, as portas publicadas incluem:

| Serviço | Porta | Finalidade |
|---|---:|---|
| Node-RED | `1880` | Editor e runtime dos fluxos |
| JaCaMo REST | `8080` | Operações REST da plataforma |
| Jason | `3272` | Endpoint configurado pela imagem JaCaMo |
| Mosquitto | `1883` | Broker MQTT |

A disponibilidade depende da configuração do host e de não haver conflitos de portas.

---

## 13. Executar clientes auxiliares

Os clientes Python são utilizados para simular dispositivos ou enviar solicitações ao agente.

Antes da execução, confira os nomes reais dos scripts disponíveis no diretório do projeto e configure o endereço do broker.

Para iniciar um cliente:

```bash
python3 AppDevice.py
```

Para iniciar o cliente de mensagens destinado ao agente:

```bash
python3 AppMsgBob.py
```

Esses comandos são exemplos para os arquivos existentes na raiz. Os scripts podem ser executados em terminais separados, conforme o cenário experimental.

O cliente de mensagens permite enviar solicitações com os campos:

```text
target operation data
```

Exemplos:

```text
device_01 setLed on
device_01 setLed off
device_02 setRelay on
device_02 setRelay off
device_03 setMotor start
device_03 setMotor stop
```

A operação `setAirconditioner` com parâmetro numérico foi utilizada em um teste separado de recepção e persistência. Ela ainda não deve ser considerada uma ação operacional completamente integrada.

---

## 14. Validação da integração

A execução realizada demonstrou os seguintes comportamentos:

| Teste | Resultado observado |
|---|---|
| Inicialização do agente Bob | Sucesso |
| Recepção de `illuminance` | Sucesso |
| Recepção de `temperature` e `humidity` | Sucesso |
| Recepção de `ledState` | Sucesso |
| Recepção de `relayState` | Sucesso |
| Recepção de `motorState` | Sucesso |
| Recepção de `lowIlluminance` | Sucesso |
| Recepção de `highIlluminance` | Sucesso |
| Recepção de `overheating` | Sucesso |
| Solicitação `setLed(on/off)` | Testada |
| Solicitação `setRelay(on/off)` | Testada |
| Solicitação `setMotor(start/stop)` | Testada |
| Persistência de observações no PostgreSQL | Testada |
| Persistência de eventos no PostgreSQL | Testada |
| Registro de solicitações de atuação | Testado |
| Preservação de parâmetro numérico `22.5` em JSONB | Testada separadamente |

Os resultados representam a validação dos cenários executados, não uma garantia de que todos os possíveis estados de falha ou concorrência tenham sido testados.

---

## 15. Estado conceitual da pesquisa

No estado atual do projeto, os dados recebidos dos dispositivos são tratados como informações provenientes do ambiente e representados como crenças do agente.

O agente atualiza crenças relativas às propriedades observadas, por exemplo:

```text
temperature(device_02, 38.2)
humidity(device_02, 49.9)
ledState(device_01, on)
relayState(device_02, off)
motorState(device_03, start)
```

A representação concreta dessas crenças depende da conversão realizada pelo artefato e dos planos definidos em `bob.asl`.

A implementação atual ainda não realiza explicitamente o processo de construção e integração de conhecimento proposto na tese.

Essa separação é intencional: primeiro consolidamos a infraestrutura de comunicação, observação e atuação; posteriormente, investigaremos os mecanismos pelos quais o agente poderá descobrir recursos, interpretar descrições, selecionar informações relevantes e construir conhecimento contextual.

---

## 16. Limitações e trabalhos futuros

Os próximos passos do projeto incluem:

1. Consolidar as Thing Descriptions dos dispositivos integrados.
2. Verificar a correspondência entre TDs, tópicos MQTT, payloads e implementações dos dispositivos.
3. Definir e documentar os tipos de dados e parâmetros de cada ação.
4. Ampliar os testes de integração para cenários de erro e indisponibilidade.
5. Avaliar mecanismos mais robustos de correlação entre solicitações e confirmações.
6. Preparar a infraestrutura para experimentos de descoberta e interpretação de descrições WoT.
7. Implementar e avaliar, posteriormente, as etapas do modelo de construção e utilização do conhecimento da tese.

A implementação de correlação explícita de confirmações por `action_id` permanece fora do escopo da versão atual.

---

## 17. Segurança e manutenção

Antes de compartilhar ou publicar o projeto:

- Remova credenciais dos arquivos de código e de configuração.
- Não versione senhas, tokens ou arquivos locais de credenciais.
- Utilize variáveis de ambiente para dados sensíveis.
- Revise as permissões de acesso ao broker MQTT.
- Evite expor portas de desenvolvimento desnecessariamente.
- Exclua ambientes virtuais, dependências instaladas, caches e arquivos de runtime do pacote de código-fonte.
- Documente as dependências necessárias para reconstruir os ambientes.

**Atenção:** não utilize `docker compose down -v` para interromper a aplicação durante o desenvolvimento normal, pois a opção `-v` remove os volumes declarados e pode apagar os dados persistidos no PostgreSQL.

Para parar os serviços sem remover os volumes:

```bash
docker compose down
```

Os dados persistidos no volume do PostgreSQL devem ser preservados conforme a configuração do Docker Compose.

---

## 18. Licença

A licença do projeto deve ser definida pelos responsáveis antes da redistribuição pública.

A licença escolhida deve ser compatível com os direitos dos autores e com as dependências utilizadas.