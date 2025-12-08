# Sistema de Calibração e Monitoramento - Máquinas de Solda

![Status](https://img.shields.io/badge/status-completo-success)
![Arduino](https://img.shields.io/badge/Arduino-Mega_2560-blue)
![Proteus](https://img.shields.io/badge/Simulação-Proteus-orange)

## 📋 Descrição do Projeto

Sistema embarcado para controle e monitoramento de máquinas de solda SMAW (eletrodo revestido) e MIG (arame contínuo), com sistema de alarmes críticos, calibração persistente e controle automático de segurança.

**Disciplina:** Microcontroladores  
**Instituição:** [Universidade Estadual do Maranhão]  
**Período:** 2025/2  
**Autor:** [Marcos Vinícius Morais Rios, Patrick Melo Albuquerque, Luís Guilherme Busaglo Lopes, Matheus Machado Santos e Suamí Gomes Santos.]

---

## 📁 Estrutura do Projeto

```text
projeto-maquina-solda/
│
├── README.md                          # Documentação principal
├── .gitignore                         # Arquivos a ignorar
│
├── hardware/                          # Arquivos de hardware
│   └── proteus/
│       ├── circuito_completo.pdsprj   # Projeto Proteus
│       └── esquematico.png            # Print do circuito
│
├── software/                          # Código Arduino
│   └── sistema_calibracao_solda/
│       └── sistema_calibracao_solda.ino  # Código principal
│
├── docs/                              # Documentação
│   ├── relatorio.md                   # Relatório técnico
│   ├── manual_usuario.md              # Manual de uso
│   └── diagramas/
│       ├── diagrama_blocos.png
│       ├── fluxograma.png
│       └── estados.png
│
├── testes/                            # Documentação de testes
│   ├── cenarios_teste.md
│   └── resultados.md
│
└── videos/                            # Vídeos de demonstração
    └── demonstracao_completa.mp4
```

---

## 🎯 Objetivos

- ✅ Monitorar parâmetros críticos de soldagem em tempo real
- ✅ Implementar sistema de alarmes com desligamento automático
- ✅ Calibração persistente de sensores (EEPROM)
- ✅ Controle automático de processos (ventilação e gás)
- ✅ Interface homem-máquina (IHM) intuitiva

---

## 🔧 Hardware

### Componentes Utilizados

| Componente | Quantidade | Função |
|---|---|---|
| Arduino Mega 2560 | 1 | Microcontrolador principal |
| Display MAX7219 | 1 | Visualização de tensão/corrente |
| LCD 16x2 I2C | 1 | Interface de usuário |
| LM35 | 1 | Sensor de temperatura |
| Potenciômetro 10kΩ | 4 | Simulação de sensores |
| Relé 5V | 5 | Controle de processos |
| Botão Push-button | 4 | Entrada de comandos |
| LED 5mm | 1 | Indicador de status |
| Resistor 220Ω | 1 | Limitador de corrente LED |

### Pinagem

#### Sensores Analógicos

- **A0:** LM35 (Temperatura)
- **A1:** Potenciômetro - Tensão (0-100V)
- **A2:** Potenciômetro - Corrente (0-500A)
- **A3:** Potenciômetro - Fluxo de Gás (0-20 L/min)
- **A4:** Potenciômetro - RPM Arame (0-3000 RPM)

#### Display MAX7219 (SPI)

- **Pino 11:** DIN (Data In)
- **Pino 13:** CLK (Clock)
- **Pino 10:** CS (Chip Select)

#### LCD 16x2 I2C

- **Pino 20:** SDA
- **Pino 21:** SCL

#### Botões (Pull-up interno)

- **Pino 2:** BTN_MENU
- **Pino 3:** BTN_UP
- **Pino 4:** BTN_DOWN
- **Pino 5:** BTN_ENTER

#### Relés

- **Pino 6:** RL1 - Sistema Principal
- **Pino 7:** RL2 - Modo SMAW/MIG
- **Pino 8:** RL3 - Ventilação
- **Pino 9:** RL4 - Válvula de Gás
- **Pino 14:** RL5 - Segurança

#### LED Status

- **Pino 12:** LED de Status

---

## 💻 Software

### Funcionalidades Implementadas

#### 1. Monitoramento em Tempo Real

- 6 telas navegáveis (BTN_MENU)
- Atualização a cada 500ms
- Visualização simultânea em 2 displays

#### 2. Sistema de Alarmes

- **Sobretensão:** > 90V
- **Sobrecorrente:** > 450A
- **Temperatura Alta:** > 80°C
- **Baixa Vazão de Gás:** < 5 L/min

**Características:**

- Desligamento automático do sistema
- Ventilação automática de emergência
- Alarmes com memória (requer ACK)
- Indicação visual (LCD + LED)
- Log em Serial Monitor

#### 3. Controle de Relés

| Relé | Tipo | Descrição |
|---|---|---|
| RL1 | Manual | Liga/desliga sistema (BTN_UP) |
| RL2 | Manual | Alterna SMAW/MIG (BTN_DOWN) |
| RL3 | Automático | Ventilação em emergências |
| RL4 | Automático | Válvula de gás (modo MIG) |
| RL5 | Fixo | Segurança (sempre ativo) |

#### 4. Menu de Calibração

- **IZERO:** Offset de corrente
- **ISPAN:** Escala corrente (0-500A)
- **VSPAN:** Escala tensão (0-100V)
- Salvamento persistente em EEPROM
- Navegação com UP/DOWN
- Edição incremental de valores

#### 5. Segurança

- Desligamento automático em alarmes críticos
- Religamento manual supervisionado
- Intertravamento de processos
- Controle automático de gás por modo

---

## 🚀 Como Usar

### Operação Normal

1. **Inicialização**
   - Sistema carrega calibração da EEPROM
   - RL5 (Segurança) liga automaticamente

2. **Ligar Sistema**
   - Pressione `BTN_UP`
   - RL1 liga, sistema operacional

3. **Selecionar Modo**
   - Pressione `BTN_DOWN` para alternar
   - SMAW: Eletrodo revestido (sem gás)
   - MIG: Arame contínuo (com gás)

4. **Navegar Telas**
   - Pressione `BTN_MENU` para alternar entre telas 0-5

### Menu de Calibração

1. **Entrar no Menu**
   - Segure `BTN_ENTER` por 3 segundos

2. **Navegar**
   - `BTN_UP`: Item anterior
   - `BTN_DOWN`: Próximo item

3. **Calibrar**
   - Selecione item com `BTN_ENTER`
   - Ajuste valor com `UP/DOWN`
   - Salve com `BTN_ENTER`

4. **Sair**
   - Navegue até "5.Sair" e confirme
   - OU pressione `BTN_MENU` para cancelar

### Em Caso de Alarme

1. Sistema desliga automaticamente
2. LCD mostra tipo de alarme
3. LED pisca rapidamente
4. Corrija o problema (ajuste RV2, RV1, etc)
5. Pressione `BTN_ENTER` para reconhecer (ACK)
6. Pressione `BTN_UP` para religar

---

## 📊 Telas do Sistema

### Tela 0 - Overview

```text
T:25C V:45V
I:320A R:1500
```

### Tela 1 - Temperatura

```text
Temperatura:
25.50 C
```

### Tela 2 - Tensão

```text
Tensao:
45.20 V
```

### Tela 3 - Corrente

```text
Corrente:
320.5 A
```

### Tela 4 - Fluxo de Gás

```text
Fluxo Gas:
12.50 L/m
```

### Tela 5 - RPM Arame

```text
RPM Arame:
1500 RPM
```

---

## 🧪 Testes Realizados

### Cenários Testados

| Teste | Status | Observação |
|---|---|---|
| Leitura de sensores | ✅ | Todos os 5 funcionando |
| Display MAX7219 | ✅ | Tensão e corrente OK |
| LCD 16x2 | ✅ | 6 telas navegáveis |
| Alarme sobretensão | ✅ | > 90V dispara |
| Alarme sobrecorrente | ✅ | > 450A dispara |
| Alarme temperatura | ✅ | > 80°C dispara |
| Alarme baixa vazão | ✅ | < 5 L/min dispara |
| Desligamento automático | ✅ | RL1 desliga |
| Ventilação emergência | ✅ | RL3 liga |
| ACK de alarmes | ✅ | BTN_ENTER funciona |
| Religamento manual | ✅ | BTN_UP após ACK |
| Menu calibração | ✅ | Navegação OK |
| Salvamento EEPROM | ✅ | Valores persistem |
| Controle modo SMAW | ✅ | RL4 desligado |
| Controle modo MIG | ✅ | RL4 ligado |
| Controle automático gás | ✅ | Responde ao fluxo |

---

## 🛠️ Compilação e Upload

### Requisitos

- Arduino IDE 1.8.19 ou superior
- Bibliotecas:
  - `LiquidCrystal_I2C`
  - `LedControl`
  - `Wire` (nativa)
  - `EEPROM` (nativa)

### Passos

1. Abra `software/sistema_calibracao_solda/sistema_calibracao_solda.ino`
2. Instale as bibliotecas necessárias
3. Selecione placa: `Arduino Mega 2560`
4. Compile (Ctrl+R)
5. Para Proteus: `Sketch > Export Compiled Binary`
6. Carregue o `.hex` no Arduino do Proteus

---

## 📚 Documentação Adicional

- [Relatório Técnico](docs/relatorio.md)
- [Manual do Usuário](docs/manual_usuario.md)
- [Cenários de Teste](testes/cenarios_teste.md)
- [Resultados dos Testes](testes/resultados.md)

---

## 🎥 Demonstração

[Link para vídeo de demonstração](videos/demonstracao_completa.mp4)

---

## 📄 Licença

Este projeto foi desenvolvido para fins educacionais na disciplina de Microcontroladores.

---

## 👤 Autores

**[Suamí Gomes Santos]**

- GitHub: [@osuamists](https://github.com/osuamists)
- Email: suamisantos34@gmail.com

---

## 📝 Changelog

### [1.0.0] - 2025-12-08

#### Adicionado

- Sistema completo de monitoramento
- 4 alarmes críticos com ACK
- Menu de calibração com EEPROM
- Controle automático de 5 relés
- 6 telas navegáveis
- Interface com 4 botões
- Comunicação serial para debug

---

## 🔮 Trabalhos Futuros (N3 - Hardware Físico)

- [ ] Montagem em protoboard
- [ ] Circuito de proteção para relés (transistor + diodo)
- [ ] PCB customizada
- [ ] Fonte de alimentação regulada
- [ ] Interface Bluetooth/WiFi
- [ ] Supervisório em Python
- [ ] Histórico de alarmes
- [ ] Perfis de soldagem salvos
- [ ] Proteção por senha

---

**Status do Projeto:** ✅ Completo e funcional (Fase N2 - Simulação)
