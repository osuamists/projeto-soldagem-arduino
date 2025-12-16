#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LedControl.h>
#include <EEPROM.h>

// ========== CONFIGURAÇÕES ==========
LiquidCrystal_I2C lcd(0x20, 16, 2);
LedControl displays = LedControl(11, 13, 10, 1);

// ========== PINOS SENSORES ==========
const int PIN_TEMP = A0;       // LM35
const int PIN_TENSAO = A1;     // POT Tensão (RV1)
const int PIN_CORRENTE = A2;   // POT Corrente (RV2)
const int PIN_FLUXO = A3;      // POT Vazão (RV3)
const int PIN_RPM = A4;        // POT RPM (RV4)

// ========== PINOS BOTÕES ==========
const int BTN_MENU = 2;
const int BTN_UP = 3;
const int BTN_DOWN = 4;
const int BTN_ENTER = 5;

// ========== PINO LED ==========
const int LED_STATUS = 12;

// ========== PINOS RELÉS ==========
const int RELE_SISTEMA = 6;      // RL1: Sistema Principal
const int RELE_MODO = 7;         // RL2: Modo SMAW/MIG
const int RELE_VENTILACAO = 8;   // RL3: Ventilação
const int RELE_GAS = 9;          // RL4: Fluxo de Gás
const int RELE_SEGURANCA = 14;   // RL5: Segurança

// ========== LIMITES DE ALARMES ==========
const float LIMITE_TENSAO_MAX = 90.0;
const float LIMITE_CORRENTE_MAX = 450.0;
const float LIMITE_TEMP_MAX = 80.0;
const float LIMITE_FLUXO_MIN = 5.0;

// ========== ESTRUTURA DE ALARMES ==========
struct Alarme {
  bool ativo;
  bool reconhecido;
  String nome;
};

Alarme alarmeSobretensao = {false, false, "SOBRETENSAO"};
Alarme alarmeSobrecorrente = {false, false, "SOBRECORRENTE"};
Alarme alarmeTempAlta = {false, false, "TEMP ALTA"};
Alarme alarmeBaixaVazao = {false, false, "BAIXA VAZAO"};

bool alarmeGlobalAtivo = false;
bool emergenciaAtiva = false;
int alarmeAtualExibido = 0;
unsigned long ultimaTrocaAlarme = 0;
const int INTERVALO_TROCA_ALARME = 2000;

// ========== VARIÁVEIS DE CALIBRAÇÃO ==========
int calibIZERO = 0;              // -50 a +50 A (offset corrente)
int calibISPAN = 500;            // 400 a 600 (ganho corrente, 100% = 500)
int calibVSPAN = 100;            // 80 a 120 (ganho tensão, 100% = 100)

// Endereços EEPROM
const int EEPROM_MAGIC = 0;      // 1 byte - validação
const int EEPROM_IZERO = 1;      // 2 bytes - int
const int EEPROM_ISPAN = 3;      // 2 bytes - int
const int EEPROM_VSPAN = 5;      // 2 bytes - int
const byte EEPROM_VALID = 0xAA;  // Valor mágico de validação

// ========== VARIÁVEIS DO MENU ==========
bool emMenuCalibracao = false;
int itemMenuCalib = 0;           // 0-4: itens do menu
int valorEditando = -1;          // -1 = navegando, 0-2 = editando

// Controle de tempo para ENTER longo (3 segundos)
unsigned long tempoInicialEnterMenu = 0;
bool enterPressionadoLongo = false;
const unsigned long TEMPO_ENTER_LONGO = 3000;  // 3 segundos

// ========== VARIÁVEIS GLOBAIS ==========
float temperatura = 0;
float tensao = 0;
float corrente = 0;
float fluxo = 0;
int rpm = 0;

// Valores de Pico (Peak Hold)
float tensaoPico = 0;
float correntePico = 0;

bool relesSistemaLigado = false;
bool relesModoSMAW = true;
bool relesVentilacao = false;
bool relesGas = false;

int telaAtual = 0;
const int totalTelas = 4;  // 0-4 = 5 telas

unsigned long ultimaAtualizacao = 0;
const int INTERVALO_ATUALIZACAO = 500;

// ========== DEBOUNCE ==========
int estadoBtnMenu = HIGH;
int ultimoEstadoLeituraMenu = HIGH;
unsigned long ultimoTempoDebounceMenu = 0;

int estadoBtnEnter = HIGH;
int ultimoEstadoLeituraEnter = HIGH;
unsigned long ultimoTempoDebounceEnter = 0;

int estadoBtnUp = HIGH;
int ultimoEstadoLeituraUp = HIGH;
unsigned long ultimoTempoDebounceUp = 0;

int estadoBtnDown = HIGH;
int ultimoEstadoLeituraDown = HIGH;
unsigned long ultimoTempoDebounceDown = 0;

const unsigned long DEBOUNCE_DELAY = 50;

unsigned long ultimoPiscaLED = 0;
bool estadoLED = false;

// ========== FUNÇÕES AUXILIARES ==========
void carregarCalibracao() {
  byte magic = EEPROM.read(EEPROM_MAGIC);
  
  Serial.println("================================================");
  Serial.println("CARREGANDO CALIBRACAO DA EEPROM...");
  
  if (magic == EEPROM_VALID) {
    EEPROM.get(EEPROM_IZERO, calibIZERO);
    EEPROM.get(EEPROM_ISPAN, calibISPAN);
    EEPROM.get(EEPROM_VSPAN, calibVSPAN);
    
    Serial.println("[OK] Calibracao carregada:");
  } else {
    Serial.println("[AVISO] EEPROM vazia - usando valores padrao:");
    calibIZERO = 0;
    calibISPAN = 500;
    calibVSPAN = 100;
  }
  
  Serial.print("  IZERO = ");
  Serial.print(calibIZERO);
  Serial.println(" A");
  Serial.print("  ISPAN = ");
  Serial.println(calibISPAN);
  Serial.print("  VSPAN = ");
  Serial.println(calibVSPAN);
  Serial.println("================================================");
}

void salvarCalibracao() {
  Serial.println("================================================");
  Serial.println("SALVANDO CALIBRACAO NA EEPROM...");
  
  EEPROM.write(EEPROM_MAGIC, EEPROM_VALID);
  EEPROM.put(EEPROM_IZERO, calibIZERO);
  EEPROM.put(EEPROM_ISPAN, calibISPAN);
  EEPROM.put(EEPROM_VSPAN, calibVSPAN);
  
  Serial.println("[OK] Calibracao salva com sucesso!");
  Serial.print("  IZERO = ");
  Serial.print(calibIZERO);
  Serial.println(" A");
  Serial.print("  ISPAN = ");
  Serial.println(calibISPAN);
  Serial.print("  VSPAN = ");
  Serial.println(calibVSPAN);
  Serial.println("================================================");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Salvando...");
  delay(500);
  lcd.setCursor(0, 1);
  lcd.print("OK! EEPROM");
  delay(1500);
  
  atualizarLCDMenu();
}

void gerenciarReleGas() {
  bool deveAbrirGas = relesSistemaLigado && 
                      !relesModoSMAW && 
                      (fluxo >= LIMITE_FLUXO_MIN) &&
                      !emergenciaAtiva;
  
  if(deveAbrirGas && !relesGas) {
    digitalWrite(RELE_GAS, HIGH);
    relesGas = true;
    Serial.println("[GAS] Valvula ABERTA");
  }
  
  if(!deveAbrirGas && relesGas) {
    digitalWrite(RELE_GAS, LOW);
    relesGas = false;
    Serial.println("[GAS] Valvula FECHADA");
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  
  delay(1000);
  Serial.println("================================================");
  Serial.println("===  SISTEMA DE CALIBRACAO - MAQUINAS SOLDA ===");
  Serial.println("================================================");
  
  pinMode(PIN_TEMP, INPUT);
  pinMode(PIN_TENSAO, INPUT);
  pinMode(PIN_CORRENTE, INPUT);
  pinMode(PIN_FLUXO, INPUT);
  pinMode(PIN_RPM, INPUT);
  
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);
  
  pinMode(LED_STATUS, OUTPUT);
  
  pinMode(RELE_SISTEMA, OUTPUT);
  pinMode(RELE_MODO, OUTPUT);
  pinMode(RELE_VENTILACAO, OUTPUT);
  pinMode(RELE_GAS, OUTPUT);
  pinMode(RELE_SEGURANCA, OUTPUT);
  
  digitalWrite(RELE_SISTEMA, LOW);
  digitalWrite(RELE_MODO, LOW);
  digitalWrite(RELE_VENTILACAO, LOW);
  digitalWrite(RELE_GAS, LOW);
  digitalWrite(RELE_SEGURANCA, HIGH);
  
  Serial.println("[OK] 5 Reles inicializados!");
  
  displays.shutdown(0, false);
  displays.setIntensity(0, 8);
  displays.clearDisplay(0);
  Serial.println("[OK] Display MAX7219 inicializado!");
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sistema Pronto!");
  Serial.println("[OK] LCD I2C inicializado!");
  
  delay(2000);
  
  carregarCalibracao();
  
  Serial.println();
  Serial.println("=== MENU DE CALIBRACAO ===");
  Serial.println("Pressione ENTER por 3s para entrar");
  Serial.println();
  
  lcd.clear();
}

// ========== LOOP ==========
void loop() {
  lerSensores();
  verificarAlarmes();
  gerenciarReleGas();
  atualizarDisplays7Seg();
  verificarBotoes();
  gerenciarLED();
  verificarSegurancaReles();
  
  if(millis() - ultimaAtualizacao > INTERVALO_ATUALIZACAO) {
    if(alarmeGlobalAtivo) {
      atualizarLCDAlarme();
    } else {
      atualizarLCD();
    }
    
    if(!emMenuCalibracao) {
      enviarDadosSerial();
    }
    
    ultimaAtualizacao = millis();
  }
}

// ========== SENSORES ==========
void lerSensores() {
  int leituraTemp = analogRead(PIN_TEMP);
  temperatura = (leituraTemp * 500.0) / 1023.0;
  
  int leituraTensao = analogRead(PIN_TENSAO);
  tensao = (leituraTensao * 100.0) / 1023.0;
  tensao = tensao * (calibVSPAN / 100.0);
  
  int leituraCorrente = analogRead(PIN_CORRENTE);
  corrente = (leituraCorrente * 500.0) / 1023.0;
  // Ajuste: Ganho primeiro, depois Offset (soma) para comportamento intuitivo
  corrente = (corrente * (calibISPAN / 500.0)) + calibIZERO;
  
  int leituraFluxo = analogRead(PIN_FLUXO);
  fluxo = (leituraFluxo * 20.0) / 1023.0;
  
  int leituraRPM = analogRead(PIN_RPM);
  rpm = (leituraRPM * 3000) / 1023;
  
  // Peak Hold - registra valores máximos
  if (tensao > tensaoPico) tensaoPico = tensao;
  if (corrente > correntePico) correntePico = corrente;
}

// ========== ALARMES ==========
void verificarAlarmes() {
  if(tensao > LIMITE_TENSAO_MAX) {
    if(!alarmeSobretensao.ativo) {
      alarmeSobretensao.ativo = true;
      alarmeSobretensao.reconhecido = false;
      Serial.println("!!! ALARME: SOBRETENSAO !!!");
    }
  } else {
    alarmeSobretensao.ativo = false;
  }
  
  if(corrente > LIMITE_CORRENTE_MAX) {
    if(!alarmeSobrecorrente.ativo) {
      alarmeSobrecorrente.ativo = true;
      alarmeSobrecorrente.reconhecido = false;
      Serial.println("!!! ALARME: SOBRECORRENTE !!!");
    }
  } else {
    alarmeSobrecorrente.ativo = false;
  }
  
  if(temperatura > LIMITE_TEMP_MAX) {
    if(!alarmeTempAlta.ativo) {
      alarmeTempAlta.ativo = true;
      alarmeTempAlta.reconhecido = false;
      Serial.println("!!! ALARME: TEMP ALTA !!!");
    }
  } else {
    alarmeTempAlta.ativo = false;
  }
  
  if(fluxo < LIMITE_FLUXO_MIN) {
    if(!alarmeBaixaVazao.ativo) {
      alarmeBaixaVazao.ativo = true;
      alarmeBaixaVazao.reconhecido = false;
      Serial.println("!!! ALARME: BAIXA VAZAO !!!");
    }
  } else {
    alarmeBaixaVazao.ativo = false;
  }
  
  alarmeGlobalAtivo = (alarmeSobretensao.ativo && !alarmeSobretensao.reconhecido) ||
                      (alarmeSobrecorrente.ativo && !alarmeSobrecorrente.reconhecido) ||
                      (alarmeTempAlta.ativo && !alarmeTempAlta.reconhecido) ||
                      (alarmeBaixaVazao.ativo && !alarmeBaixaVazao.reconhecido);
}

void atualizarLCDAlarme() {
  int numAlarmes = 0;
  Alarme* alarmes[4];
  
  if(alarmeSobrecorrente.ativo && !alarmeSobrecorrente.reconhecido) alarmes[numAlarmes++] = &alarmeSobrecorrente;
  if(alarmeSobretensao.ativo && !alarmeSobretensao.reconhecido) alarmes[numAlarmes++] = &alarmeSobretensao;
  if(alarmeTempAlta.ativo && !alarmeTempAlta.reconhecido) alarmes[numAlarmes++] = &alarmeTempAlta;
  if(alarmeBaixaVazao.ativo && !alarmeBaixaVazao.reconhecido) alarmes[numAlarmes++] = &alarmeBaixaVazao;
  
  if(numAlarmes == 0) return;
  
  if(millis() - ultimaTrocaAlarme > INTERVALO_TROCA_ALARME) {
    alarmeAtualExibido++;
    if(alarmeAtualExibido >= numAlarmes) alarmeAtualExibido = 0;
    ultimaTrocaAlarme = millis();
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("!!! ALARME !!!");
  lcd.setCursor(0, 1);
  lcd.print(alarmes[alarmeAtualExibido]->nome);
  
  if(numAlarmes > 1) {
    lcd.setCursor(14, 0);
    lcd.print("(");
    lcd.print(numAlarmes);
    lcd.print(")");
  }
}

void reconhecerAlarmes() {
  if(alarmeSobretensao.ativo) alarmeSobretensao.reconhecido = true;
  if(alarmeSobrecorrente.ativo) alarmeSobrecorrente.reconhecido = true;
  if(alarmeTempAlta.ativo) alarmeTempAlta.reconhecido = true;
  if(alarmeBaixaVazao.ativo) alarmeBaixaVazao.reconhecido = true;
  
  alarmeGlobalAtivo = false;
  alarmeAtualExibido = 0;
  emergenciaAtiva = false;
  
  // Força desligamento imediato de ventilação e segurança
  digitalWrite(RELE_VENTILACAO, LOW);
  digitalWrite(RELE_SEGURANCA, HIGH);
  relesVentilacao = false;
  
  Serial.println("[ACK] Alarmes reconhecidos - Ventilacao DESLIGADA");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alarmes ACK");
  lcd.setCursor(0, 1);
  lcd.print("Use UP p/ ligar");
  delay(2000);
  atualizarLCD();
}

void verificarSegurancaReles() {
  if((alarmeSobrecorrente.ativo && !alarmeSobrecorrente.reconhecido) ||
     (alarmeSobretensao.ativo && !alarmeSobretensao.reconhecido)) {
    
    if(!emergenciaAtiva) {
      emergenciaAtiva = true;
      Serial.println("!!! EMERGENCIA ATIVADA !!!");
    }
    
    digitalWrite(RELE_SISTEMA, LOW);
    digitalWrite(RELE_GAS, LOW);
    digitalWrite(RELE_VENTILACAO, HIGH); // Liga ventilação
    digitalWrite(RELE_SEGURANCA, HIGH);
    
    relesSistemaLigado = false;
    relesGas = false;
    relesVentilacao = true;
  }
  else {
    // Se não há emergência, garante que relés de segurança desligam
    if(emergenciaAtiva) {
      Serial.println("[EMERGENCIA] Condition cleared - desligando relés de segurança");
      emergenciaAtiva = false;
    }
    
    // Desliga ventilação se não há emergência
    if(relesVentilacao) {
      digitalWrite(RELE_VENTILACAO, LOW);
      relesVentilacao = false;
      Serial.println("[VENTILACAO] Desligada");
    }
    
    // Relé de segurança volta ao normal (HIGH = desativado)
    digitalWrite(RELE_SEGURANCA, HIGH);
  }
}

void gerenciarLED() {
  int intervalo = alarmeGlobalAtivo ? 100 : 500;
  
  if(millis() - ultimoPiscaLED > intervalo) {
    estadoLED = !estadoLED;
    digitalWrite(LED_STATUS, estadoLED);
    ultimoPiscaLED = millis();
  }
}

// ========== DISPLAYS ==========
void atualizarDisplays7Seg() {
  int tensaoInt = (int)(tensao * 10);
  if(tensaoInt > 999) tensaoInt = 999;
  
  displays.setDigit(0, 5, (tensaoInt / 100) % 10, false);
  displays.setDigit(0, 6, (tensaoInt / 10) % 10, true);
  displays.setDigit(0, 7, tensaoInt % 10, false);
  displays.setChar(0, 4, ' ', false);
  
  int correnteInt = (int)corrente;
  if(correnteInt > 999) correnteInt = 999;
  
  displays.setDigit(0, 0, (correnteInt / 100) % 10, false);
  displays.setDigit(0, 1, (correnteInt / 10) % 10, false);
  displays.setDigit(0, 2, correnteInt % 10, false);
  displays.setChar(0, 3, ' ', false);
}

void atualizarLCD() {
  if (emMenuCalibracao) {
    atualizarLCDMenu();
    return;
  }
  
  lcd.clear();
  
  switch (telaAtual) {
    case 0:
      // TELA 0: Resumo - V, I, Modo, Status
      lcd.print("V:");
      lcd.print(tensao, 0);
      lcd.print(" I:");
      lcd.print(corrente, 0);
      lcd.print("A");
      lcd.setCursor(0, 1);
      lcd.print(relesModoSMAW ? "SMAW " : "MIG  ");
      lcd.print(relesSistemaLigado ? "[ON] " : "[OFF]");
      break;
      
    case 1:
      // TELA 1: Tensão Detalhada (RMS, Avg, Pico)
      // Para DC: RMS = Avg = Instantâneo
      lcd.print("Vr:");
      lcd.print(tensao, 0);
      lcd.print(" Va:");
      lcd.print(tensao, 0);
      lcd.setCursor(0, 1);
      lcd.print("Vpk:");
      lcd.print(tensaoPico, 1);
      lcd.print("V");
      break;
      
    case 2:
      // TELA 2: Corrente Detalhada (RMS, Avg, Pico)
      lcd.print("Ir:");
      lcd.print(corrente, 0);
      lcd.print(" Ia:");
      lcd.print(corrente, 0);
      lcd.setCursor(0, 1);
      lcd.print("Ipk:");
      lcd.print(correntePico, 0);
      lcd.print("A");
      break;
      
    case 3:
      // TELA 3: Processo - Temperatura e Fluxo
      lcd.print("Temp:");
      lcd.print(temperatura, 1);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("Fluxo:");
      lcd.print(fluxo, 1);
      lcd.print("L/m");
      break;
      
    case 4:
      // TELA 4: Motor - RPM
      lcd.print("Motor Arame");
      lcd.setCursor(0, 1);
      lcd.print(rpm);
      lcd.print(" RPM");
      break;
  }
  
  lcd.setCursor(15, 0);
  lcd.print(telaAtual);
}

void enviarDadosSerial() {
  // Formato: DATA,tempo,temp,tensao,corrente,fluxo,rpm,tensaoPico,correntePico
  Serial.print("DATA,");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(temperatura, 2);
  Serial.print(",");
  Serial.print(tensao, 2);
  Serial.print(",");
  Serial.print(corrente, 1);
  Serial.print(",");
  Serial.print(fluxo, 2);
  Serial.print(",");
  Serial.print(rpm);
  Serial.print(",");
  Serial.print(tensaoPico, 2);
  Serial.print(",");
  Serial.println(correntePico, 1);
}

// ========== BOTÕES ==========
void verificarBotoes() {
  // MENU
  int leituraMenu = digitalRead(BTN_MENU);
  if (leituraMenu != ultimoEstadoLeituraMenu) {
    ultimoTempoDebounceMenu = millis();
  }
  
  if (millis() - ultimoTempoDebounceMenu > DEBOUNCE_DELAY) {
    if (leituraMenu != estadoBtnMenu) {
      estadoBtnMenu = leituraMenu;
      
      if (estadoBtnMenu == LOW && !alarmeGlobalAtivo && !emMenuCalibracao) {
        telaAtual++;
        if(telaAtual > totalTelas) telaAtual = 0;
        atualizarLCD();
        Serial.print("MENU: Tela ");
        Serial.println(telaAtual);
      }
    }
  }
  ultimoEstadoLeituraMenu = leituraMenu;
  
  // ENTER - COM DETECÇÃO LONGA
  int leituraEnter = digitalRead(BTN_ENTER);
  if (leituraEnter != ultimoEstadoLeituraEnter) {
    ultimoTempoDebounceEnter = millis();
  }
  
  if (millis() - ultimoTempoDebounceEnter > DEBOUNCE_DELAY) {
    if (leituraEnter != estadoBtnEnter) {
      estadoBtnEnter = leituraEnter;
      
      // PRESSIONADO
      if (estadoBtnEnter == LOW) {
        tempoInicialEnterMenu = millis();
        enterPressionadoLongo = false;
        
        if (!emMenuCalibracao) {
          Serial.println("ENTER pressionado (segure 3s para menu)...");
        }
      }
      
      // LIBERADO
      if (estadoBtnEnter == HIGH) {
        unsigned long tempoPressionado = millis() - tempoInicialEnterMenu;
        
        // LONGO (≥ 3s)
        if (tempoPressionado >= TEMPO_ENTER_LONGO && !emMenuCalibracao) {
          Serial.println(">>> ENTER LONGO - Menu calibracao!");
          entrarMenuCalibracao();
        }
        // CURTO (< 3s)
        else if (tempoPressionado < TEMPO_ENTER_LONGO) {
          if (emMenuCalibracao) {
            selecionarItemMenu();
          } else if (alarmeGlobalAtivo) {
            reconhecerAlarmes();
          } else {
            // Reset dos valores de pico
            tensaoPico = tensao;
            correntePico = corrente;
            lcd.clear();
            lcd.print("Picos Resetados");
            lcd.setCursor(0, 1);
            lcd.print("Vpk/Ipk = atual");
            Serial.println("[RESET] Picos resetados");
            delay(1000);
            atualizarLCD();
          }
        }
        
        tempoInicialEnterMenu = 0;
      }
    }
  }
  
  // DETECTA 3 SEGUNDOS ENQUANTO SEGURA - mostra progresso
  if (estadoBtnEnter == LOW && !enterPressionadoLongo && !emMenuCalibracao) {
    unsigned long tempoPressionado = millis() - tempoInicialEnterMenu;
    
    // Mostra feedback visual durante a contagem
    if (tempoPressionado >= 500 && tempoPressionado < TEMPO_ENTER_LONGO) {
      int segundos = (tempoPressionado / 1000) + 1;
      lcd.setCursor(0, 0);
      lcd.print("Segure ENTER... ");
      lcd.setCursor(0, 1);
      lcd.print(segundos);
      lcd.print("s / 3s         ");
    }
    
    if (tempoPressionado >= TEMPO_ENTER_LONGO) {
      enterPressionadoLongo = true;
      Serial.println(">>> 3 SEGUNDOS!");
      
      lcd.clear();
      lcd.print("Abrindo Menu");
      lcd.setCursor(0, 1);
      lcd.print("Calibracao...");
    }
  }
  
  ultimoEstadoLeituraEnter = leituraEnter;
  
  // UP
  int leituraUp = digitalRead(BTN_UP);
  if (leituraUp != ultimoEstadoLeituraUp) {
    ultimoTempoDebounceUp = millis();
  }
  
  if (millis() - ultimoTempoDebounceUp > DEBOUNCE_DELAY) {
    if (leituraUp != estadoBtnUp) {
      estadoBtnUp = leituraUp;
      
      if (estadoBtnUp == LOW) {
        if (emMenuCalibracao) {
          navegarMenu(true);
        } else if (!alarmeGlobalAtivo) {
          relesSistemaLigado = !relesSistemaLigado;
          digitalWrite(RELE_SISTEMA, relesSistemaLigado ? HIGH : LOW);
          
          lcd.clear();
          lcd.print("Sistema");
          lcd.setCursor(0, 1);
          lcd.print(relesSistemaLigado ? "LIGADO" : "DESLIGADO");
          Serial.print("UP: Sistema ");
          Serial.println(relesSistemaLigado ? "LIGADO" : "DESLIGADO");
          delay(1000);
          
          if (!emMenuCalibracao) atualizarLCD();
        }
      }
    }
  }
  ultimoEstadoLeituraUp = leituraUp;
  
  // DOWN
  int leituraDown = digitalRead(BTN_DOWN);
  if (leituraDown != ultimoEstadoLeituraDown) {
    ultimoTempoDebounceDown = millis();
  }
  
  if (millis() - ultimoTempoDebounceDown > DEBOUNCE_DELAY) {
    if (leituraDown != estadoBtnDown) {
      estadoBtnDown = leituraDown;
      
      if (estadoBtnDown == LOW) {
        if (emMenuCalibracao) {
          navegarMenu(false);
        } else if (!alarmeGlobalAtivo) {
          relesModoSMAW = !relesModoSMAW;
          digitalWrite(RELE_MODO, relesModoSMAW ? HIGH : LOW);
          
          lcd.clear();
          lcd.print("Modo");
          lcd.setCursor(0, 1);
          lcd.print(relesModoSMAW ? "SMAW" : "MIG");
          Serial.print("DOWN: Modo ");
          Serial.println(relesModoSMAW ? "SMAW" : "MIG");
          delay(1000);
          
          if (!emMenuCalibracao) atualizarLCD();
        }
      }
    }
  }
  ultimoEstadoLeituraDown = leituraDown;
}

// ========== MENU DE CALIBRAÇÃO ==========
void entrarMenuCalibracao() {
  // Permitir calibracao mesmo com alarmes (manutencao)
  // Reconhece alarmes automaticamente ao entrar no menu
  if (alarmeGlobalAtivo) {
    reconhecerAlarmes();
  }
  
  emMenuCalibracao = true;
  itemMenuCalib = 0;
  valorEditando = -1;
  
  Serial.println();
  Serial.println("=============================");
  Serial.println("  MENU DE CALIBRACAO ATIVO  ");
  Serial.println("=============================");
  
  delay(1000);
  atualizarLCDMenu();
}

void atualizarLCDMenu() {
  lcd.clear();
  
  if (valorEditando == -1) {
    lcd.print(">>> CALIBRACAO");
    lcd.setCursor(0, 1);
    
    switch (itemMenuCalib) {
      case 0:
        lcd.print("1.IZERO:");
        lcd.print(calibIZERO);
        break;
      case 1:
        lcd.print("2.ISPAN:");
        lcd.print(calibISPAN);
        break;
      case 2:
        lcd.print("3.VSPAN:");
        lcd.print(calibVSPAN);
        break;
      case 3:
        lcd.print("4.Salvar EEPROM");
        break;
      case 4:
        lcd.print("5.Sair");
        break;
    }
  } else {
    lcd.print("EDITANDO:");
    lcd.setCursor(0, 1);
    
    switch (valorEditando) {
      case 0:
        lcd.print("IZERO:");
        lcd.print(calibIZERO);
        break;
      case 1:
        lcd.print("ISPAN:");
        lcd.print(calibISPAN);
        break;
      case 2:
        lcd.print("VSPAN:");
        lcd.print(calibVSPAN);
        break;
    }
    
    lcd.setCursor(15, 1);
    lcd.print((millis() / 500) % 2 ? "<" : " ");
  }
}

void navegarMenu(bool subir) {
  if (valorEditando != -1) {
    editarValor(subir);
  } else {
    if (subir) {
      itemMenuCalib--;
      if (itemMenuCalib < 0) itemMenuCalib = 4;
    } else {
      itemMenuCalib++;
      if (itemMenuCalib > 4) itemMenuCalib = 0;
    }
    
    Serial.print("Item: ");
    Serial.println(itemMenuCalib);
    atualizarLCDMenu();
  }
}

void selecionarItemMenu() {
  if (valorEditando != -1) {
    valorEditando = -1;
    Serial.println("Valor confirmado!");
    atualizarLCDMenu();
  } else {
    switch (itemMenuCalib) {
      case 0:
      case 1:
      case 2:
        valorEditando = itemMenuCalib;
        Serial.print("Editando: ");
        Serial.println(itemMenuCalib);
        atualizarLCDMenu();
        break;
        
      case 3:
        salvarCalibracao();
        break;
        
      case 4:
        sairMenuCalibracao();
        break;
    }
  }
}

void editarValor(bool incrementar) {
  switch (valorEditando) {
    case 0:  // IZERO (-50 a +50)
      calibIZERO += incrementar ? 1 : -1;
      if (calibIZERO > 50) calibIZERO = 50;
      if (calibIZERO < -50) calibIZERO = -50;
      break;
      
    case 1:  // ISPAN (400 a 600)
      calibISPAN += incrementar ? 5 : -5;
      if (calibISPAN > 600) calibISPAN = 600;
      if (calibISPAN < 400) calibISPAN = 400;
      break;
      
    case 2:  // VSPAN (80 a 120)
      calibVSPAN += incrementar ? 1 : -1;
      if (calibVSPAN > 120) calibVSPAN = 120;
      if (calibVSPAN < 80) calibVSPAN = 80;
      break;
  }
  
  atualizarLCDMenu();
  Serial.print("IZERO=");
  Serial.print(calibIZERO);
  Serial.print(" ISPAN=");
  Serial.print(calibISPAN);
  Serial.print(" VSPAN=");
  Serial.println(calibVSPAN);
}

void sairMenuCalibracao() {
  emMenuCalibracao = false;
  valorEditando = -1;
  
  lcd.clear();
  lcd.print("Saindo...");
  delay(1000);
  
  Serial.println(">>> SAIU DO MENU");
  atualizarLCD();
}
