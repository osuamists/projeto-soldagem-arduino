# 📌 Diagrama de Pinagem - Sistema de Calibração de Máquinas de Solda

## 🔌 Arduino Mega 2560 - Pinout Completo

### 📊 Tabela de Conexões

| Pino | Tipo | Componente | Sinal | Descrição |
|---|---|---|---|---|
| **A0** | Analógico | LM35 | Vout | Sensor de temperatura (0-100°C) |
| **A1** | Analógico | RV1 (POT 10kΩ) | Wiper | Simulação tensão (0-100V) |
| **A2** | Analógico | RV2 (POT 10kΩ) | Wiper | Simulação corrente (0-500A) |
| **A3** | Analógico | RV3 (POT 10kΩ) | Wiper | Simulação fluxo gás (0-20 L/min) |
| **A4** | Analógico | RV4 (POT 10kΩ) | Wiper | Simulação RPM arame (0-3000 RPM) |
| **2** | Digital | BTN_MENU | Input | Botão navegação telas (Pull-up) |
| **3** | Digital | BTN_UP | Input | Botão UP / Liga sistema (Pull-up) |
| **4** | Digital | BTN_DOWN | Input | Botão DOWN / Modo SMAW/MIG (Pull-up) |
| **5** | Digital | BTN_ENTER | Input | Botão ENTER / ACK alarmes (Pull-up) |
| **6** | Digital | RL1 (Relé) | Output | Sistema Principal ON/OFF |
| **7** | Digital | RL2 (Relé) | Output | Modo SMAW (OFF) / MIG (ON) |
| **8** | Digital | RL3 (Relé) | Output | Ventilação emergência |
| **9** | Digital | RL4 (Relé) | Output | Válvula de gás (automático) |
| **10** | Digital | MAX7219 | CS | Chip Select display 7-seg |
| **11** | Digital | MAX7219 | DIN | Data In display 7-seg (SPI) |
| **12** | Digital | LED Status | Output | LED indicador status (220Ω) |
| **13** | Digital | MAX7219 | CLK | Clock display 7-seg (SPI) |
| **14** | Digital | RL5 (Relé) | Output | Segurança (sempre ON) |
| **20** | Digital | LCD I2C | SDA | Dados I2C (LCD 16x2) |
| **21** | Digital | LCD I2C | SCL | Clock I2C (LCD 16x2) |
| **5V** | Power | Múltiplos | VCC | Alimentação componentes |
| **GND** | Power | Múltiplos | GND | Terra comum |

---

## 🖼️ Diagrama Visual ASCII

```text
╔══════════════════════════════════════════════════════════════════════════════╗
║                        ARDUINO MEGA 2560 - PINAGEM COMPLETA                  ║
╚══════════════════════════════════════════════════════════════════════════════╝

SENSORES ANALÓGICOS              BOTÕES DE ENTRADA              RELÉS E SAÍDAS
─────────────────────            ──────────────────             ──────────────
LM35          ─────────┬── A0     BTN_MENU      ────┬── 2      RL1 (Sistema)  ───┬── 6
RV1 (Tensão)  ─────────┤── A1     BTN_UP        ────┤── 3      RL2 (Modo)     ───┤── 7
RV2 (Corrente)─────────┤── A2     BTN_DOWN      ────┤── 4      RL3 (Ventil.)  ───┤── 8
RV3 (Fluxo)   ─────────┤── A3     BTN_ENTER     ────┤── 5      RL4 (Gás)      ───┤── 9
RV4 (RPM)     ─────────┴── A4                        │          RL5 (Segur.)   ───┴── 14
                                                     │
                                                     │
          ╔════════════════════════════════════════════════════════════════╗
          ║              DISPLAYS E COMUNICAÇÃO                             ║
          ╚════════════════════════════════════════════════════════════════╝
          
          MAX7219 (Display 7-seg)      LCD I2C (16x2)         SPI/I2C
          ──────────────────────       ───────────────        ───────
          CS  ─────────────────── 10   SDA ───────────── 20   CLK ─── 13
          DIN ─────────────────── 11   SCL ───────────── 21   MOSI ── 11
          CLK ─────────────────── 13
          
          ╔════════════════════════════════════════════════════════════════╗
          ║                    ALIMENTAÇÃO                                  ║
          ╚════════════════════════════════════════════════════════════════╝
          
          +5V ──────────────────► VCC (Componentes)
          GND ──────────────────► GND (Terra comum)
          
          LED Status ───► 12 (com resistor 220Ω para GND)
```

---

## 📐 Conexões Detalhadas por Subsistema

### 🌡️ Subsistema de Sensores

#### LM35 (Temperatura)

```text
LM35
┌─────┐
│ 1 ├────► 5V (VCC)
│ 2 ├────► A0 (Vout)
│ 3 ├────► GND
└─────┘
```

#### Potenciômetros (4x)

```text
POT 10kΩ
┌─────┐
│ 1 ├────► 5V
│ 2 ├────► A1/A2/A3/A4 (Wiper)
│ 3 ├────► GND
└─────┘

RV1 → A1 (Tensão)
RV2 → A2 (Corrente)
RV3 → A3 (Fluxo Gás)
RV4 → A4 (RPM Arame)
```

---

### 🖥️ Subsistema de Displays

#### Display MAX7219 (8 dígitos 7-seg)

```text
MAX7219
┌─────────┐
│ VCC ├────► 5V
│ GND ├────► GND
│ DIN ├────► Pino 11 (MOSI)
│ CS ├────► Pino 10
│ CLK ├────► Pino 13 (SCK)
└─────────┘

Layout: [CCC] [VVV]
^^^ ^^^
Corrente Tensão

```

#### LCD 16x2 I2C

```text
LCD I2C (Endereço 0x20)
┌─────────┐
│ VCC ├────► 5V
│ GND ├────► GND
│ SDA ├────► Pino 20
│ SCL ├────► Pino 21
└─────────┘

Layout: [16 caracteres]
[16 caracteres]
```

---

### 🔘 Subsistema de Entrada (Botões)

text
    Botão (4x) com Pull-up Interno
    ┌─────┐
    │     │
Pino ───┤ ● ├─── GND
│ │
└─────┘

BTN_MENU → Pino 2 (INPUT_PULLUP)
BTN_UP → Pino 3 (INPUT_PULLUP)
BTN_DOWN → Pino 4 (INPUT_PULLUP)
BTN_ENTER → Pino 5 (INPUT_PULLUP)

Lógica: HIGH = não pressionado
LOW = pressionado

text

---

### 🔌 Subsistema de Relés

Relé Nativo Proteus (5x)
┌──────────┐
│ COIL+ ├────► Pino Digital
│ COIL- ├────► GND
│ │
│ COM │ (não conectado na simulação)
│ NO │ (não conectado na simulação)
│ NC │ (não conectado na simulação)
└──────────┘

Tensão Bobina: 5V
Lógica: HIGH = Relé LIGADO
LOW = Relé DESLIGADO

RL1 → Pino 6 (Sistema Principal)
RL2 → Pino 7 (Modo SMAW/MIG)
RL3 → Pino 8 (Ventilação)
RL4 → Pino 9 (Válvula Gás)
RL5 → Pino 14 (Segurança)

text

---

### 💡 Subsistema de Indicação (LED)

text
    LED Status
    ┌─────┐
Pino 12 ─┤>├───[220Ω]─── GND
└─────┘

Comportamento:

Pisca LENTO (500ms): Sistema normal

Pisca RÁPIDO (100ms): Alarme ativo

text

---

## ⚡ Alimentação

Fonte de Alimentação
5V ────► VCC (múltiplos componentes)
GND ────► GND (terra comum)

Consumo Estimado:

Arduino Mega: ~50 mA

MAX7219: ~100 mA

LCD I2C: ~20 mA

LM35: ~60 µA

LED: ~10 mA

Relés (5x): 0 mA (simulação)
───────────────────
TOTAL: ~180 mA

text

---

## 🔧 Notas de Implementação

### ⚠️ Importante - Simulação vs Físico

#### Na Simulação (Proteus)

- ✅ Relés conectados **diretamente** aos pinos
- ✅ Não precisa transistor/diodo
- ✅ Pull-up interno dos botões funciona

#### No Hardware Físico (N3)

- ⚠️ Relés precisam de **transistor BC547** + **diodo 1N4007**
- ⚠️ Resistor **1kΩ** na base do transistor
- ⚠️ Fonte de alimentação externa para relés

---

## 📋 Checklist de Conexões

### Antes de Ligar

- [ ] Todos os GNDs conectados (terra comum)
- [ ] Alimentação 5V nos componentes corretos
- [ ] LM35 com polaridade correta (Vout no meio)
- [ ] Endereço I2C do LCD verificado (0x20 ou 0x27)
- [ ] Botões com pull-up ativado no código
- [ ] Tensão dos relés configurada para 5V no Proteus
- [ ] MAX7219 com DIN, CLK, CS corretos
- [ ] Nenhum curto-circuito visível

---

## 🐛 Troubleshooting

| Problema | Possível Causa | Solução |
|---|---|---|
| LCD em branco | Endereço I2C errado | Testar 0x20 ou 0x27 |
| Display 7-seg apagado | SPI mal conectado | Verificar pinos 10,11,13 |
| Botões não respondem | Pull-up não ativado | Usar INPUT_PULLUP |
| Relés não acionam | Tensão bobina errada | Configurar 5V no Proteus |
| Leitura analógica errada | Referência errada | Verificar fórmulas no código |
| LM35 lendo errado | Pinos trocados | Vout no pino central |

---

**Última atualização:** 08/12/2025  
**Versão:** 1.0
