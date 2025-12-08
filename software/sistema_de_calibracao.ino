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


// ========== ENDEREÇOS EEPROM ==========
const int EEPROM_IZERO_ADDR = 0;
const int EEPROM_ISPAN_ADDR = 4;
const int EEPROM_VSPAN_ADDR = 8;
const int EEPROM_FLAG_ADDR = 12;
const long EEPROM_VALID_FLAG = 0xCAFEBABE;  // Flag de validação

// ========== VARIÁVEIS DE CALIBRAÇÃO ==========
float calibIZERO = 0.0;      // Offset de corrente
float calibISPAN = 500.0;    // Fundo de escala corrente
float calibVSPAN = 100.0;    // Fundo de escala tensão

bool emModoEdicao = false;
float valorEmEdicao = 0.0;
unsigned long tempoEnterpressedMenu = 0;
bool enterPressionadoLongo = false;

// ========== NOMES DOS ITENS DO MENU ==========
const String itensMenu[] = {
  "1.IZERO Calib",
  "2.ISPAN Calib",
  "3.VSPAN Calib",
  "4.Ver Valores",
  "5.Sair"
};

// ========== VARIÁVEIS DO MENU ==========
bool emMenuCalibracao = false;
int itemMenuAtual = 0;
const int totalItensMenu = 4;  // IZERO, ISPAN, VSPAN, VER VALORES, SAIR

// ========== VARIÁVEIS GLOBAIS ==========
float temperatura = 0;
float tensao = 0;
float corrente = 0;
float fluxo = 0;
int rpm = 0;

bool relesSistemaLigado = false;
bool relesModoSMAW = true;
bool relesVentilacao = false;
bool relesGas = false;

int telaAtual = 0;
const int totalTelas = 5;

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


// ========== FUNÇÕES EEPROM ==========
void salvarFloatEEPROM(int endereco, float valor) {
  byte* p = (byte*)(void*)&valor;
  for (int i = 0; i < sizeof(valor); i++) {
    EEPROM.write(endereco + i, *p++);
  }
}

float lerFloatEEPROM(int endereco) {
  float valor;
  byte* p = (byte*)(void*)&valor;
  for (int i = 0; i < sizeof(valor); i++) {
    *p++ = EEPROM.read(endereco + i);
  }
  return valor;
}

void salvarLongEEPROM(int endereco, long valor) {
  byte* p = (byte*)(void*)&valor;
  for (int i = 0; i < sizeof(valor); i++) {
    EEPROM.write(endereco + i, *p++);
  }
}

long lerLongEEPROM(int endereco) {
  long valor;
  byte* p = (byte*)(void*)&valor;
  for (int i = 0; i < sizeof(valor); i++) {
    *p++ = EEPROM.read(endereco + i);
  }
  return valor;
}

void carregarCalibracao() {
  Serial.println("================================================");
  Serial.println("CARREGANDO CALIBRACAO DA EEPROM...");
  
  long flag = lerLongEEPROM(EEPROM_FLAG_ADDR);
  
  if(flag == EEPROM_VALID_FLAG) {
    calibIZERO = lerFloatEEPROM(EEPROM_IZERO_ADDR);
    calibISPAN = lerFloatEEPROM(EEPROM_ISPAN_ADDR);
    calibVSPAN = lerFloatEEPROM(EEPROM_VSPAN_ADDR);
    
    Serial.println("[OK] Calibracao carregada:");
    Serial.print("  IZERO: "); Serial.println(calibIZERO, 2);
    Serial.print("  ISPAN: "); Serial.println(calibISPAN, 2);
    Serial.print("  VSPAN: "); Serial.println(calibVSPAN, 2);
  } else {
    // Valores padrão
    calibIZERO = 0.0;
    calibISPAN = 500.0;
    calibVSPAN = 100.0;
    
    Serial.println("[AVISO] Calibracao nao encontrada - usando valores padrao");
    Serial.print("  IZERO: "); Serial.println(calibIZERO, 2);
    Serial.print("  ISPAN: "); Serial.println(calibISPAN, 2);
    Serial.print("  VSPAN: "); Serial.println(calibVSPAN, 2);
  }
  Serial.println("================================================");
}

void salvarCalibracao() {
  Serial.println("================================================");
  Serial.println("SALVANDO CALIBRACAO NA EEPROM...");
  
  salvarFloatEEPROM(EEPROM_IZERO_ADDR, calibIZERO);
  salvarFloatEEPROM(EEPROM_ISPAN_ADDR, calibISPAN);
  salvarFloatEEPROM(EEPROM_VSPAN_ADDR, calibVSPAN);
  salvarLongEEPROM(EEPROM_FLAG_ADDR, EEPROM_VALID_FLAG);
  
  Serial.println("[OK] Calibracao salva com sucesso!");
  Serial.print("  IZERO: "); Serial.println(calibIZERO, 2);
  Serial.print("  ISPAN: "); Serial.println(calibISPAN, 2);
  Serial.print("  VSPAN: "); Serial.println(calibVSPAN, 2);
  Serial.println("================================================");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibracao");
  lcd.setCursor(0, 1);
  lcd.print("SALVA!");
  delay(1500);
}

void aplicarCalibracao() {
  // Aplica IZERO (offset)
  corrente = corrente - calibIZERO;
  if(corrente < 0) corrente = 0;
  
  // Aplica ISPAN (ajuste de escala)
  corrente = (corrente / 500.0) * calibISPAN;
  
  // Aplica VSPAN (ajuste de escala)
  tensao = (tensao / 100.0) * calibVSPAN;
}

// ========== GERENCIAMENTO AUTOMÁTICO DO RELÉ DE GÁS ==========
void gerenciarReleGas() {
  // Condições para abrir válvula de gás:
  // 1. Sistema deve estar ligado
  // 2. Modo MIG ativo (não SMAW)
  // 3. Fluxo de gás adequado
  // 4. Não pode estar em emergência
  
  bool deveAbrirGas = relesSistemaLigado && 
                      !relesModoSMAW && 
                      (fluxo >= LIMITE_FLUXO_MIN) &&
                      !emergenciaAtiva;
  
  // Abre válvula se necessário
  if(deveAbrirGas && !relesGas) {
    digitalWrite(RELE_GAS, HIGH);
    relesGas = true;
    
    Serial.println("================================================");
    Serial.println("[GAS] Valvula de gas ABERTA");
    Serial.print("[GAS] Modo: MIG | Fluxo: ");
    Serial.print(fluxo, 2);
    Serial.println(" L/min");
    Serial.println("================================================");
  }
  
  // Fecha válvula se necessário
  if(!deveAbrirGas && relesGas) {
    digitalWrite(RELE_GAS, LOW);
    relesGas = false;
    
    Serial.println("================================================");
    Serial.println("[GAS] Valvula de gas FECHADA");
    
    // Diagnóstico do motivo
    if(!relesSistemaLigado) {
      Serial.println("[GAS] Motivo: Sistema desligado");
    } else if(relesModoSMAW) {
      Serial.println("[GAS] Motivo: Modo SMAW (nao requer gas)");
    } else if(fluxo < LIMITE_FLUXO_MIN) {
      Serial.println("[GAS] Motivo: Fluxo insuficiente (< 5 L/min)");
    } else if(emergenciaAtiva) {
      Serial.println("[GAS] Motivo: Emergencia ativa");
    }
    
    Serial.println("================================================");
  }
}


// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  
  // Teste de comunicação serial
  delay(1000);
  Serial.println("================================================");
  Serial.println("===  SISTEMA DE CALIBRACAO - MAQUINAS SOLDA ===");
  Serial.println("================================================");
  Serial.println("Hardware: Arduino Mega 2560");
  Serial.println("Displays: MAX7219 + LCD 16x2 I2C");
  Serial.println("Reles: 5 unidades (controle direto)");
  Serial.println("Sensores: LM35 + 4 POTs");
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
  
  // ========== RELÉS (LÓGICA NORMAL) ==========
  pinMode(RELE_SISTEMA, OUTPUT);
  pinMode(RELE_MODO, OUTPUT);
  pinMode(RELE_VENTILACAO, OUTPUT);
  pinMode(RELE_GAS, OUTPUT);
  pinMode(RELE_SEGURANCA, OUTPUT);
  
  digitalWrite(RELE_SISTEMA, LOW);       // OFF
  digitalWrite(RELE_MODO, LOW);          // OFF
  digitalWrite(RELE_VENTILACAO, LOW);    // OFF
  digitalWrite(RELE_GAS, LOW);           // OFF
  digitalWrite(RELE_SEGURANCA, HIGH);    // ON (sempre ativo)
  
  Serial.println("[OK] 5 Reles nativos inicializados!");
  
  displays.shutdown(0, false);
  displays.setIntensity(0, 8);
  displays.clearDisplay(0);
  Serial.println("[OK] Display MAX7219 inicializado!");
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sistema Pronto!");
  lcd.setCursor(0, 1);
  lcd.print("Inicializando...");
  Serial.println("[OK] LCD I2C inicializado!");
  
  delay(2000);
  lcd.clear();
  
  Serial.println("================================================");
  Serial.println("LIMITES DE ALARME:");
  Serial.print("  Tensao maxima: "); Serial.print(LIMITE_TENSAO_MAX); Serial.println(" V");
  Serial.print("  Corrente maxima: "); Serial.print(LIMITE_CORRENTE_MAX); Serial.println(" A");
  Serial.print("  Temperatura maxima: "); Serial.print(LIMITE_TEMP_MAX); Serial.println(" C");
  Serial.print("  Fluxo minimo: "); Serial.print(LIMITE_FLUXO_MIN); Serial.println(" L/min");
  Serial.println("================================================");
  Serial.println("[OK] Sistema 100% inicializado!");
  Serial.println("================================================");

  carregarCalibracao();  // Carrega valores da EEPROM
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Segure ENTER 3s");
  lcd.setCursor(0, 1);
  lcd.print("p/ Calibracao");
  delay(2000);
  lcd.clear();
  
  Serial.println("[INFO] Segure ENTER por 3s para entrar no menu de calibracao");
}

// ========== LOOP ==========
void loop() {
  lerSensores();
  aplicarCalibracao(); 
  verificarAlarmes();
  gerenciarReleGas();
  atualizarDisplays7Seg();

  
  if(emMenuCalibracao) {
    verificarBotoesMenu();
  } else {
    verificarBotoes();
    verificarEntradaMenu();  // Verifica ENTER longo
  }
  
  gerenciarLED();
  verificarSegurancaReles();
  
  if(millis() - ultimaAtualizacao > INTERVALO_ATUALIZACAO) {
    if(emMenuCalibracao) {
      atualizarLCDMenu();
    } else if(alarmeGlobalAtivo) {
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
  temperatura = (analogRead(PIN_TEMP) * 500.0) / 1023.0;
  tensao = (analogRead(PIN_TENSAO) * 100.0) / 1023.0;
  corrente = (analogRead(PIN_CORRENTE) * 500.0) / 1023.0;
  fluxo = (analogRead(PIN_FLUXO) * 20.0) / 1023.0;
  rpm = (analogRead(PIN_RPM) * 3000) / 1023;
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
    alarmeSobretensao.reconhecido = false;
  }
  
  if(corrente > LIMITE_CORRENTE_MAX) {
    if(!alarmeSobrecorrente.ativo) {
      alarmeSobrecorrente.ativo = true;
      alarmeSobrecorrente.reconhecido = false;
      Serial.println("!!! ALARME: SOBRECORRENTE !!!");
    }
  } else {
    alarmeSobrecorrente.ativo = false;
    alarmeSobrecorrente.reconhecido = false;
  }
  
  if(temperatura > LIMITE_TEMP_MAX) {
    if(!alarmeTempAlta.ativo) {
      alarmeTempAlta.ativo = true;
      alarmeTempAlta.reconhecido = false;
      Serial.println("!!! ALARME: TEMP ALTA !!!");
    }
  } else {
    alarmeTempAlta.ativo = false;
    alarmeTempAlta.reconhecido = false;
  }
  
  if(fluxo < LIMITE_FLUXO_MIN) {
    if(!alarmeBaixaVazao.ativo) {
      alarmeBaixaVazao.ativo = true;
      alarmeBaixaVazao.reconhecido = false;
      Serial.println("!!! ALARME: BAIXA VAZAO !!!");
    }
  } else {
    alarmeBaixaVazao.ativo = false;
    alarmeBaixaVazao.reconhecido = false;
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
  bool algumReconhecido = false;
  
  if(alarmeSobretensao.ativo && !alarmeSobretensao.reconhecido) {
    alarmeSobretensao.reconhecido = true;
    Serial.println("[ACK] Alarme SOBRETENSAO reconhecido");
    algumReconhecido = true;
  }
  if(alarmeSobrecorrente.ativo && !alarmeSobrecorrente.reconhecido) {
    alarmeSobrecorrente.reconhecido = true;
    Serial.println("[ACK] Alarme SOBRECORRENTE reconhecido");
    algumReconhecido = true;
  }
  if(alarmeTempAlta.ativo && !alarmeTempAlta.reconhecido) {
    alarmeTempAlta.reconhecido = true;
    Serial.println("[ACK] Alarme TEMP ALTA reconhecido");
    algumReconhecido = true;
  }
  if(alarmeBaixaVazao.ativo && !alarmeBaixaVazao.reconhecido) {
    alarmeBaixaVazao.reconhecido = true;
    Serial.println("[ACK] Alarme BAIXA VAZAO reconhecido");
    algumReconhecido = true;
  }
  
  if(algumReconhecido) {
    alarmeGlobalAtivo = false;
    alarmeAtualExibido = 0;
    emergenciaAtiva = false;
    
    // Desliga ventilação de emergência
    digitalWrite(RELE_VENTILACAO, LOW);
    relesVentilacao = false;
    
    // RL1 (Sistema) permanece DESLIGADO - requer religamento MANUAL
    // Operador deve pressionar BTN_UP para religar
    
    Serial.println("================================================");
    Serial.println("[INFO] Alarmes reconhecidos");
    Serial.println("[INFO] Sistema aguarda religamento MANUAL");
    Serial.println("[INFO] Pressione BTN_UP para religar sistema");
    Serial.println("================================================");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Alarmes ACK");
    lcd.setCursor(0, 1);
    lcd.print("Use UP p/ ligar");
    delay(2000);
    atualizarLCD();
  }
}

void verificarSegurancaReles() {
  // Só ativa emergência se houver alarme crítico NÃO reconhecido
  if((alarmeSobrecorrente.ativo && !alarmeSobrecorrente.reconhecido) ||
     (alarmeSobretensao.ativo && !alarmeSobretensao.reconhecido)) {
    
    if(!emergenciaAtiva) {
      emergenciaAtiva = true;
      
      Serial.println("================================================");
      Serial.println("!!!          EMERGENCIA ATIVADA             !!!");
      Serial.println("!!! Sistema desligado automaticamente       !!!");
      Serial.println("================================================");
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("** EMERGENCIA **");
      lcd.setCursor(0, 1);
      lcd.print("Sistema DESLIG.");
      delay(2000);
    }
    
    // Força relés no estado de segurança
    digitalWrite(RELE_SISTEMA, LOW);       // Sistema OFF
    digitalWrite(RELE_GAS, LOW);           // Gás OFF
    digitalWrite(RELE_VENTILACAO, HIGH);   // Ventilação ON
    digitalWrite(RELE_SEGURANCA, HIGH);    // Segurança ON
    
    relesSistemaLigado = false;
    relesGas = false;
    relesVentilacao = true;
  }
  // REMOVER O ELSE COMPLETAMENTE!
  // A ventilação só desliga quando o operador fizer ACK
}


void gerenciarLED() {
  if(alarmeGlobalAtivo) {
    if(millis() - ultimoPiscaLED > 100) {
      estadoLED = !estadoLED;
      digitalWrite(LED_STATUS, estadoLED);
      ultimoPiscaLED = millis();
    }
  } else {
    if(millis() - ultimoPiscaLED > 500) {
      estadoLED = !estadoLED;
      digitalWrite(LED_STATUS, estadoLED);
      ultimoPiscaLED = millis();
    }
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
  lcd.clear();
  switch(telaAtual) {
    case 0:
      lcd.print("T:");
      lcd.print(temperatura, 0);
      lcd.print("C V:");
      lcd.print(tensao, 0);
      lcd.print("V");
      lcd.setCursor(0, 1);
      lcd.print("I:");
      lcd.print(corrente, 0);
      lcd.print("A R:");
      lcd.print(rpm);
      break;
    case 1:
      lcd.print("Temperatura:");
      lcd.setCursor(0, 1);
      lcd.print(temperatura, 2);
      lcd.print(" C");
      break;
    case 2:
      lcd.print("Tensao:");
      lcd.setCursor(0, 1);
      lcd.print(tensao, 2);
      lcd.print(" V");
      break;
    case 3:
      lcd.print("Corrente:");
      lcd.setCursor(0, 1);
      lcd.print(corrente, 1);
      lcd.print(" A");
      break;
    case 4:
      lcd.print("Fluxo Gas:");
      lcd.setCursor(0, 1);
      lcd.print(fluxo, 2);
      lcd.print(" L/m");
      break;
    case 5:
      lcd.print("RPM Arame:");
      lcd.setCursor(0, 1);
      lcd.print(rpm);
      lcd.print(" RPM");
      break;
  }
  lcd.setCursor(15, 0);
  lcd.print(telaAtual);
}

// ========== ENTRADA NO MENU ==========
void verificarEntradaMenu() {
  int leituraEnter = digitalRead(BTN_ENTER);
  
  // Detecta quando ENTER é pressionado
  if(leituraEnter == LOW && !enterPressionadoLongo) {
    if(tempoEnterpressedMenu == 0) {
      tempoEnterpressedMenu = millis();
    }
    
    // Se segurou por 3 segundos
    if(millis() - tempoEnterpressedMenu > 3000) {
      enterPressionadoLongo = true;
      emMenuCalibracao = true;
      itemMenuAtual = 0;
      
      Serial.println("================================================");
      Serial.println("ENTRANDO NO MENU DE CALIBRACAO");
      Serial.println("================================================");
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Menu Calibracao");
      lcd.setCursor(0, 1);
      lcd.print("Carregando...");
      delay(1000);
      atualizarLCDMenu();
    }
  } else {
    // Resetar quando soltar o botão
    tempoEnterpressedMenu = 0;
    if(leituraEnter == HIGH) {
      enterPressionadoLongo = false;
    }
  }
}

// ========== ATUALIZAR LCD DO MENU ==========
void atualizarLCDMenu() {
  if(emModoEdicao) {
    // Modo de edição de valor
    lcd.clear();
    lcd.setCursor(0, 0);
    
    if(itemMenuAtual == 0) lcd.print("IZERO:");
    else if(itemMenuAtual == 1) lcd.print("ISPAN:");
    else if(itemMenuAtual == 2) lcd.print("VSPAN:");
    
    lcd.setCursor(0, 1);
    lcd.print(valorEmEdicao, 2);
    lcd.print(" ");
    
    // Indicador de edição piscante
    if((millis() / 500) % 2 == 0) {
      lcd.print("<EDIT>");
    }
  } else {
    // Menu de navegação
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(itensMenu[itemMenuAtual]);
    
    lcd.setCursor(0, 1);
    
    if(itemMenuAtual == 0) {
      lcd.print("Atual:");
      lcd.print(calibIZERO, 1);
    } else if(itemMenuAtual == 1) {
      lcd.print("Atual:");
      lcd.print(calibISPAN, 1);
    } else if(itemMenuAtual == 2) {
      lcd.print("Atual:");
      lcd.print(calibVSPAN, 1);
    } else if(itemMenuAtual == 3) {
      lcd.print("Ver calibracao");
    } else if(itemMenuAtual == 4) {
      lcd.print("Voltar ao menu");
    }
  }
}

// ========== BOTÕES NO MENU ==========
void verificarBotoesMenu() {
  // UP - Navegar/Incrementar
  int leituraUp = digitalRead(BTN_UP);
  if (leituraUp != ultimoEstadoLeituraUp) ultimoTempoDebounceUp = millis();
  if ((millis() - ultimoTempoDebounceUp) > DEBOUNCE_DELAY) {
    if (leituraUp != estadoBtnUp) {
      estadoBtnUp = leituraUp;
      if (estadoBtnUp == LOW) {
        if(emModoEdicao) {
          // Incrementar valor
          valorEmEdicao += 1.0;
          if(valorEmEdicao > 999.0) valorEmEdicao = 999.0;
          Serial.print("[MENU] Valor: ");
          Serial.println(valorEmEdicao, 2);
        } else {
          // Navegar para cima
          itemMenuAtual--;
          if(itemMenuAtual < 0) itemMenuAtual = totalItensMenu;
          Serial.print("[MENU] Item: ");
          Serial.println(itensMenu[itemMenuAtual]);
          atualizarLCDMenu();
        }
      }
    }
  }
  ultimoEstadoLeituraUp = leituraUp;
  
  // DOWN - Navegar/Decrementar
  int leituraDown = digitalRead(BTN_DOWN);
  if (leituraDown != ultimoEstadoLeituraDown) ultimoTempoDebounceDown = millis();
  if ((millis() - ultimoTempoDebounceDown) > DEBOUNCE_DELAY) {
    if (leituraDown != estadoBtnDown) {
      estadoBtnDown = leituraDown;
      if (estadoBtnDown == LOW) {
        if(emModoEdicao) {
          // Decrementar valor
          valorEmEdicao -= 1.0;
          if(valorEmEdicao < 0.0) valorEmEdicao = 0.0;
          Serial.print("[MENU] Valor: ");
          Serial.println(valorEmEdicao, 2);
        } else {
          // Navegar para baixo
          itemMenuAtual++;
          if(itemMenuAtual > totalItensMenu) itemMenuAtual = 0;
          Serial.print("[MENU] Item: ");
          Serial.println(itensMenu[itemMenuAtual]);
          atualizarLCDMenu();
        }
      }
    }
  }
  ultimoEstadoLeituraDown = leituraDown;
  
  // ENTER - Selecionar/Confirmar
  int leituraEnter = digitalRead(BTN_ENTER);
  if (leituraEnter != ultimoEstadoLeituraEnter) ultimoTempoDebounceEnter = millis();
  if ((millis() - ultimoTempoDebounceEnter) > DEBOUNCE_DELAY) {
    if (leituraEnter != estadoBtnEnter) {
      estadoBtnEnter = leituraEnter;
      if (estadoBtnEnter == LOW) {
        if(emModoEdicao) {
          // Salvar valor editado
          if(itemMenuAtual == 0) calibIZERO = valorEmEdicao;
          else if(itemMenuAtual == 1) calibISPAN = valorEmEdicao;
          else if(itemMenuAtual == 2) calibVSPAN = valorEmEdicao;
          
          salvarCalibracao();
          emModoEdicao = false;
          atualizarLCDMenu();
        } else {
          // Selecionar item do menu
          if(itemMenuAtual == 0) {
            // Calibrar IZERO
            valorEmEdicao = calibIZERO;
            emModoEdicao = true;
            Serial.println("[MENU] Editando IZERO");
          } else if(itemMenuAtual == 1) {
            // Calibrar ISPAN
            valorEmEdicao = calibISPAN;
            emModoEdicao = true;
            Serial.println("[MENU] Editando ISPAN");
          } else if(itemMenuAtual == 2) {
            // Calibrar VSPAN
            valorEmEdicao = calibVSPAN;
            emModoEdicao = true;
            Serial.println("[MENU] Editando VSPAN");
          } else if(itemMenuAtual == 3) {
            // Ver valores
            mostrarValoresCalibrados();
          } else if(itemMenuAtual == 4) {
            // Sair
            emMenuCalibracao = false;
            Serial.println("[MENU] Saindo do menu de calibracao");
            lcd.clear();
            lcd.print("Saindo...");
            delay(1000);
            atualizarLCD();
          }
        }
      }
    }
  }
  ultimoEstadoLeituraEnter = leituraEnter;
  
  // MENU - Cancelar/Voltar
  int leituraMenu = digitalRead(BTN_MENU);
  if (leituraMenu != ultimoEstadoLeituraMenu) ultimoTempoDebounceMenu = millis();
  if ((millis() - ultimoTempoDebounceMenu) > DEBOUNCE_DELAY) {
    if (leituraMenu != estadoBtnMenu) {
      estadoBtnMenu = leituraMenu;
      if (estadoBtnMenu == LOW) {
        if(emModoEdicao) {
          // Cancelar edição
          emModoEdicao = false;
          Serial.println("[MENU] Edicao cancelada");
          atualizarLCDMenu();
        } else {
          // Sair do menu
          emMenuCalibracao = false;
          Serial.println("[MENU] Menu cancelado");
          lcd.clear();
          lcd.print("Cancelado");
          delay(1000);
          atualizarLCD();
        }
      }
    }
  }
  ultimoEstadoLeituraMenu = leituraMenu;
}

// ========== MOSTRAR VALORES ==========
void mostrarValoresCalibrados() {
  Serial.println("================================================");
  Serial.println("VALORES DE CALIBRACAO ATUAIS:");
  Serial.print("  IZERO: "); Serial.print(calibIZERO, 2); Serial.println(" A");
  Serial.print("  ISPAN: "); Serial.print(calibISPAN, 2); Serial.println(" A");
  Serial.print("  VSPAN: "); Serial.print(calibVSPAN, 2); Serial.println(" V");
  Serial.println("================================================");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IZ:");
  lcd.print(calibIZERO, 1);
  lcd.print(" IS:");
  lcd.print((int)calibISPAN);
  
  lcd.setCursor(0, 1);
  lcd.print("VS:");
  lcd.print((int)calibVSPAN);
  lcd.print(" ");
  
  delay(3000);
  atualizarLCDMenu();
}

// ========== BOTÕES ==========
void verificarBotoes() {
  // MENU
  int leituraMenu = digitalRead(BTN_MENU);
  if (leituraMenu != ultimoEstadoLeituraMenu) ultimoTempoDebounceMenu = millis();
  if ((millis() - ultimoTempoDebounceMenu) > DEBOUNCE_DELAY) {
    if (leituraMenu != estadoBtnMenu) {
      estadoBtnMenu = leituraMenu;
      if (estadoBtnMenu == LOW && !alarmeGlobalAtivo) {
        telaAtual++;
        if(telaAtual > totalTelas) telaAtual = 0;
        atualizarLCD();
        Serial.print("[MENU] Tela: ");
        Serial.println(telaAtual);
      }
    }
  }
  ultimoEstadoLeituraMenu = leituraMenu;
  
  // ENTER
  int leituraEnter = digitalRead(BTN_ENTER);
  if (leituraEnter != ultimoEstadoLeituraEnter) ultimoTempoDebounceEnter = millis();
  if ((millis() - ultimoTempoDebounceEnter) > DEBOUNCE_DELAY) {
    if (leituraEnter != estadoBtnEnter) {
      estadoBtnEnter = leituraEnter;
      if (estadoBtnEnter == LOW) {
        if(alarmeGlobalAtivo) {
          reconhecerAlarmes();
        } else {
          Serial.println("[ENTER] Pressionado (sem acao)");
        }
      }
    }
  }
  ultimoEstadoLeituraEnter = leituraEnter;
  
  // UP
  int leituraUp = digitalRead(BTN_UP);
  if (leituraUp != ultimoEstadoLeituraUp) ultimoTempoDebounceUp = millis();
  if ((millis() - ultimoTempoDebounceUp) > DEBOUNCE_DELAY) {
    if (leituraUp != estadoBtnUp) {
      estadoBtnUp = leituraUp;
      if (estadoBtnUp == LOW && !alarmeGlobalAtivo) {
        relesSistemaLigado = !relesSistemaLigado;
        digitalWrite(RELE_SISTEMA, relesSistemaLigado ? HIGH : LOW);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sistema:");
        lcd.setCursor(0, 1);
        lcd.print(relesSistemaLigado ? "LIGADO" : "DESLIGADO");
        
        Serial.println("================================================");
        Serial.print("[UP] Sistema: ");
        Serial.println(relesSistemaLigado ? "LIGADO" : "DESLIGADO");
        Serial.println("================================================");
        
        delay(1000);
        atualizarLCD();
      }
    }
  }
  ultimoEstadoLeituraUp = leituraUp;
  
  // DOWN
  int leituraDown = digitalRead(BTN_DOWN);
  if (leituraDown != ultimoEstadoLeituraDown) ultimoTempoDebounceDown = millis();
  if ((millis() - ultimoTempoDebounceDown) > DEBOUNCE_DELAY) {
    if (leituraDown != estadoBtnDown) {
      estadoBtnDown = leituraDown;
      if (estadoBtnDown == LOW && !alarmeGlobalAtivo) {
        relesModoSMAW = !relesModoSMAW;
        digitalWrite(RELE_MODO, relesModoSMAW ? HIGH : LOW);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Modo:");
        lcd.setCursor(0, 1);
        lcd.print(relesModoSMAW ? "SMAW" : "MIG");
        
        Serial.println("================================================");
        Serial.print("[DOWN] Modo: ");
        Serial.println(relesModoSMAW ? "SMAW" : "MIG");
        Serial.println("================================================");
        
        delay(1000);
        atualizarLCD();
      }
    }
  }
  ultimoEstadoLeituraDown = leituraDown;
}

// ========== SERIAL ==========
void enviarDadosSerial() {
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
  Serial.print(",RELES:");
  Serial.print(relesSistemaLigado ? "1" : "0");
  Serial.print(relesModoSMAW ? "S" : "M");
  Serial.print(relesVentilacao ? "1" : "0");
  Serial.print(relesGas ? "1" : "0");
  Serial.print(",ALARM:");
  
  if(alarmeGlobalAtivo) {
    if(alarmeSobrecorrente.ativo && !alarmeSobrecorrente.reconhecido) Serial.print("I,");
    if(alarmeSobretensao.ativo && !alarmeSobretensao.reconhecido) Serial.print("V,");
    if(alarmeTempAlta.ativo && !alarmeTempAlta.reconhecido) Serial.print("T,");
    if(alarmeBaixaVazao.ativo && !alarmeBaixaVazao.reconhecido) Serial.print("F,");
  } else {
    Serial.print("OK");
  }
  
  Serial.println();
}
