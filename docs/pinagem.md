# 📌 Documentação de Pinagem - Sistema de Calibração

> **Autores:** Suamí Santos, Luís Guilherme, Patrick Melo, Marcos Vinícius e Matheus Machado  
> **Última atualização:** 27/11/2025  
> **Microcontrolador:** Arduino UNO (ATmega328P)

---

## 📊 Resumo de Utilização dos Pinos

| Tipo | Pinos Utilizados | Pinos Disponíveis |
|------|------------------|-------------------|
| Analógicos | A0, A1, A2, A3, A4, A5 | - |
| Digitais | D2, D3, D4, D5, D13 | D6, D7, D8, D9, D10, D11, D12 |
| I2C | A4 (SDA), A5 (SCL) | - |
| Serial | D0 (RX), D1 (TX) | - (evitar usar) |

---

## 🌡️ Entradas Analógicas

| Componente | Pino | Faixa | Fórmula de Conversão | Status |
|------------|------|-------|----------------------|--------|
| LM35 (temperatura) | A0 | 0-100°C | `(valor * 5.0 / 1023.0) / 0.01` | ✅ TESTADO |
| RV1 (tensão) | A1 | 0-50V | `(valor * 5.0 / 1023.0) * 10.0` | ✅ TESTADO |
| RV3 (corrente) | A2 | 0-500A | `(valor * 5.0 / 1023.0) * 100.0` | ✅ TESTADO |
| RV4 (fluxo gás) | A3 | 0-20 L/min | `(valor * 5.0 / 1023.0) * 4.0` | ✅ TESTADO |

### Detalhamento dos Sensores

#### 🌡️ LM35 - Sensor de Temperatura (A0)

- **Função:** Monitoramento da temperatura da peça/ambiente
- **Tensão de operação:** 4V a 30V
- **Sensibilidade:** 10mV/°C
- **Precisão:** ±0.5°C (a 25°C)
- **Conexão:**
  - Pino 1 (VCC) → 5V
  - Pino 2 (OUT) → A0
  - Pino 3 (GND) → GND

#### ⚡ RV1 - Simulador de Tensão de Soldagem (A1)

- **Função:** Simula a leitura do sensor de tensão do arco
- **Faixa simulada:** 0V a 50V
- **Resolução:** ~0.049V por incremento ADC
- **Aplicação real:** Substituir por divisor de tensão adequado

#### 🔌 RV3 - Simulador de Corrente de Soldagem (A2)

- **Função:** Simula a leitura do sensor de corrente
- **Faixa simulada:** 0A a 500A
- **Resolução:** ~0.49A por incremento ADC
- **Aplicação real:** Substituir por sensor de efeito Hall (ex: ACS712)

#### 💨 RV4 - Simulador de Fluxo de Gás (A3)

- **Função:** Simula a leitura do sensor de vazão de gás
- **Faixa simulada:** 0 a 20 L/min
- **Resolução:** ~0.02 L/min por incremento ADC
- **Aplicação real:** Substituir por sensor de fluxo (ex: YF-S201)

---

## 🖥️ Interface I2C

| Componente | Pino | Endereço I2C | Status |
|------------|------|--------------|--------|
| PCF8574 (LCD 16x2) | A4 (SDA), A5 (SCL) | 0x20 | ✅ FUNCIONANDO |

### LCD 16x2 via PCF8574

- **Display:** LCD 16 colunas x 2 linhas
- **Interface:** I2C através do expansor PCF8574
- **Endereço padrão:** 0x20 (pode variar: 0x27, 0x3F)
- **Biblioteca:** LiquidCrystal_I2C

```cpp
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x20, 16, 2);
```

---

## 🔘 Botões (Interface Local)

| Botão | Pino | Resistor Pull-Down | Função | Status |
|-------|------|--------------------|--------|--------|
| BTN_MENU | D2 | R2 (10kΩ) | Navegação entre telas | ✅ TESTADO |
| BTN_UP | D3 | R3 (10kΩ) | Incrementa valores | ✅ TESTADO |
| BTN_DOWN | D4 | R4 (10kΩ) | Decrementa valores | ✅ TESTADO |
| BTN_ENTER | D5 | R5 (10kΩ) | Confirma seleção | ✅ TESTADO |

### Configuração dos Botões

```cpp
#define BTN_MENU   2
#define BTN_UP     3
#define BTN_DOWN   4
#define BTN_ENTER  5

void setup() {
  pinMode(BTN_MENU, INPUT);   // Pull-down externo (R2 = 10kΩ)
  pinMode(BTN_UP, INPUT);     // Pull-down externo (R3 = 10kΩ)
  pinMode(BTN_DOWN, INPUT);   // Pull-down externo (R4 = 10kΩ)
  pinMode(BTN_ENTER, INPUT);  // Pull-down externo (R5 = 10kΩ)
}
```

> **Nota:** Os botões usam resistores pull-down externos de 10kΩ. Lógica: HIGH = pressionado, LOW = solto.

---

## 💡 Saídas Digitais

| Componente | Pino | Observação | Status |
|------------|------|------------|--------|
| LED teste | D13 | Resistor 220Ω (R1) | ✅ FUNCIONANDO |

### Configuração das Saídas

```cpp
#define LED_TESTE  13

void setup() {
  pinMode(LED_TESTE, OUTPUT);
  digitalWrite(LED_TESTE, LOW);
}
```

---

## 📡 Comunicação Serial

| Interface | Pinos | Baud Rate | Função |
|-----------|-------|-----------|--------|
| Serial (USB) | D0 (RX), D1 (TX) | 9600 | Comunicação com supervisório Python |

### Protocolo de Comunicação

- **Formato:** String separada por vírgula
- **Envio:** `TEMP,TENSAO,CORRENTE,FLUXO,STATUS\n`
- **Exemplo:** `25.5,32.1,180.0,12.5,OK\n`

---

## 🔓 Pinos Disponíveis

| Pino | Tipo | Observação |
|------|------|------------|
| D6 | Digital/PWM | Livre para expansão |
| D7 | Digital | Livre para expansão |
| D8 | Digital | Livre para expansão |
| D9 | Digital/PWM | Livre para expansão |
| D10 | Digital/PWM | Livre para expansão |
| D11 | Digital/PWM | Livre para expansão |
| D12 | Digital | Livre para expansão |

> **⚠️ Evitar usar:** D0 e D1 (reservados para comunicação Serial/USB)

---

## 🔌 Diagrama de Conexões

```text
                    ARDUINO UNO
                   +------------+
              A0 --|            |-- D13 (LED_TESTE + R1)
    LM35 ----→     |            |
              A1 --|            |-- D12 (livre)
    RV1  ----→     |            |
              A2 --|            |-- D11 (livre)
    RV3  ----→     |            |
              A3 --|            |-- D10 (livre)
    RV4  ----→     |            |
              A4 --|            |-- D9  (livre)
    SDA  ←---→     |            |
              A5 --|            |-- D8  (livre)
    SCL  ←---→     |            |
                   |            |-- D7  (livre)
              5V --|            |-- D6  (livre)
                   |            |-- D5  (BTN_ENTER + R5)
             GND --|            |-- D4  (BTN_DOWN + R4)
                   |            |-- D3  (BTN_UP + R3)
             VIN --|            |-- D2  (BTN_MENU + R2)
                   |            |-- D1  (TX) ⚠️
                   |            |-- D0  (RX) ⚠️
                   +------------+
```

---

## ⚠️ Notas Importantes

1. **Alimentação:** O Arduino deve ser alimentado com fonte externa de 7-12V para suportar todos os componentes
2. **Resistores Pull-Down:** Todos os botões usam resistores de 10kΩ para GND (R2, R3, R4, R5)
3. **LED:** O LED de teste usa resistor de 220Ω (R1) em série
4. **Sensores reais:** Os potenciômetros (RV1, RV3, RV4) são simuladores - substituir por sensores adequados na versão final
5. **I2C:** Verificar endereço do PCF8574 com scanner I2C se não funcionar
6. **Serial:** Evitar usar D0 e D1 para outros fins - reservados para comunicação USB

---

## 📋 Checklist de Testes

- [x] LM35 - Leitura de temperatura
- [x] RV1 - Simulação de tensão
- [x] RV3 - Simulação de corrente
- [x] RV4 - Simulação de fluxo
- [x] LCD I2C - Display funcionando
- [ ] Botões - Entradas digitais
- [ ] Relés - Saídas de potência
- [ ] PWM - Saídas analógicas
- [ ] Serial - Comunicação com PC
