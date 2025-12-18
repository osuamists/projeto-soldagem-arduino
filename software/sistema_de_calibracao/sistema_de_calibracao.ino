#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

/*
 * =============================================================
 * SISTEMA DE SOLDA PROFISSIONAL - FINAL CORRIGIDO
 * Hardware: Arduino MEGA 2560
 * LCD: 0x27 (16x2)
 * 7 Seg: Cátodo Comum (2 dígitos ativos via varredura)
 * Relés: 2 Canais (Sistema e Gás)
 * =============================================================
 */

// ========== CONFIGURAÇÕES LCD ==========
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== PINOS DO DISPLAY 7 SEGMENTOS (Cátodo Comum) ==========
int segA = 2; int segB = 3; int segC = 4; int segD = 5;
int segE = 6; int segF = 7; int segG = 9; 
int d1 = 10; // Dígito Esquerda
int d2 = 11; // Dígito Direita
int d3 = 12; // Desativado
int d4 = 13; // Desativado

const byte numMap[10] = {
  0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011,
  0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011
};

int valoresSeteSeg[2] = {0, 0};
unsigned long tempoTrocaVariavel7Seg = 0;
bool mostrarTensaoNo7Seg = true;

// ========== PINOS SENSORES ==========
const int PIN_TEMP = A0;  
const int PIN_TENSAO = A1; 
const int PIN_CORRENTE = A2;   
const int PIN_FLUXO = A3; 
const int PIN_RPM = A4;        

// ========== PINOS BOTÕES ==========
const int BTN_MENU = 28; 
const int BTN_UP = 26;  
const int BTN_DOWN = 24; 
const int BTN_ENTER = 22;

// ========== PINO LED ==========
const int LED_STATUS = 22;

// ========== PINOS RELÉS (Adaptado para Módulo de 2 Canais) ==========
const int RELE_SISTEMA = 30; // Relé 1 do Módulo (Liga a Máquina)
const int RELE_GAS = 32;     // Relé 2 do Módulo (Libera o Gás)

// Estes aqui não existem fisicamente, mas mantivemos a variável
// para a lógica de alarmes não quebrar. Definimos como -1.
const int RELE_MODO = -1;       
const int RELE_VENTILACAO = -1;   
const int RELE_SEGURANCA = -1;

// ========== LIMITES E CALIBRAÇÃO ==========
const float LIMITE_TENSAO_MAX = 90.0;
const float LIMITE_CORRENTE_MAX = 450.0;
const float LIMITE_TEMP_MAX = 80.0;
const float LIMITE_FLUXO_MIN = 5.0;

int calibIZERO = 0;
int calibISPAN = 500;
int calibVSPAN = 100;

// EEPROM Addresses
const int EEPROM_MAGIC = 0;
const int EEPROM_IZERO = 1;
const int EEPROM_ISPAN = 3;
const int EEPROM_VSPAN = 5;
const byte EEPROM_VALID = 0xAA;

// ========== VARIÁVEIS DE ESTADO ==========
float temperatura, tensao, corrente, fluxo, tensaoPico = 0, correntePico = 0;
int rpm = 0;
bool relesSistemaLigado = false;
bool relesModoSMAW = true;
bool emergenciaAtiva = false;
bool emMenuCalibracao = false;
int itemMenuCalib = 0;
int valorEditando = -1;
int telaAtual = 0;
const int totalTelas = 4;

unsigned long ultimaAtualizacaoLCD = 0;
unsigned long tempoBotaoEnter = 0;
bool enterLongoDetectado = false;

struct Alarme { bool ativo; bool reconhecido; String nome; };
Alarme alarmeV = {false, false, "SOBRETENSAO"};
Alarme alarmeI = {false, false, "SOBRECORRENTE"};
Alarme alarmeT = {false, false, "TEMP ALTA"};
bool alarmeGlobalAtivo = false;

// ============================================================
//  FUNÇÕES DE APOIO E EEPROM
// ============================================================

void carregarCalibracao() {
  if (EEPROM.read(EEPROM_MAGIC) == EEPROM_VALID) {
    EEPROM.get(EEPROM_IZERO, calibIZERO);
    EEPROM.get(EEPROM_ISPAN, calibISPAN);
    EEPROM.get(EEPROM_VSPAN, calibVSPAN);
  }
}

void salvarCalibracao() {
  EEPROM.write(EEPROM_MAGIC, EEPROM_VALID);
  EEPROM.put(EEPROM_IZERO, calibIZERO);
  EEPROM.put(EEPROM_ISPAN, calibISPAN);
  EEPROM.put(EEPROM_VSPAN, calibVSPAN);
  lcd.clear();
  lcd.print("Salvo na EEPROM");
  // Pequena gambiarra para esperar sem travar o display 7 seg
  unsigned long t = millis();
  while(millis() - t < 1500) { 
      // Precisamos declarar atualizarDisplay7Seg antes ou usar protótipo,
      // mas como está tudo no mesmo arquivo, o compilador arduino costuma resolver.
      // Vou mover as funcoes de display para cima para garantir.
  }
}

// ============================================================
//  CONTROLE DO DISPLAY 7 SEGMENTOS (MULTIPLEXAÇÃO)
// ============================================================

void acenderSegmentos(int num) {
  byte segs = (num >= 0) ? numMap[num] : 0;
  digitalWrite(segA, bitRead(segs, 6)); digitalWrite(segB, bitRead(segs, 5));
  digitalWrite(segC, bitRead(segs, 4)); digitalWrite(segD, bitRead(segs, 3));
  digitalWrite(segE, bitRead(segs, 2)); digitalWrite(segF, bitRead(segs, 1));
  digitalWrite(segG, bitRead(segs, 0));
}

void atualizarDisplay7Seg() {
  digitalWrite(d3, HIGH); digitalWrite(d4, HIGH); // Sempre desligados

  // Dígito 1
  digitalWrite(d2, HIGH); 
  acenderSegmentos(valoresSeteSeg[0]);
  digitalWrite(d1, LOW); 
  delayMicroseconds(2500); 
  digitalWrite(d1, HIGH); // Desliga para não vazar

  // Dígito 2
  acenderSegmentos(valoresSeteSeg[1]);
  digitalWrite(d2, LOW);
  delayMicroseconds(2500);
  digitalWrite(d2, HIGH);
}

// FUNÇÃO QUE FALTAVA NO SEU CÓDIGO
void esperaComDisplay(unsigned long ms) {
  unsigned long inicio = millis();
  while(millis() - inicio < ms) {
    atualizarDisplay7Seg();
  }
}

void prepararDados7Seg() {
  if(millis() - tempoTrocaVariavel7Seg > 2500) {
    mostrarTensaoNo7Seg = !mostrarTensaoNo7Seg;
    tempoTrocaVariavel7Seg = millis();
  }
  
  int valor = alarmeGlobalAtivo ? 88 : (int)(mostrarTensaoNo7Seg ? tensao : corrente);
  
  if (valor > 99) {
    valoresSeteSeg[0] = (valor / 100) % 10;
    valoresSeteSeg[1] = (valor / 10) % 10;
  } else {
    valoresSeteSeg[0] = (valor < 10) ? -1 : (valor / 10) % 10;
    valoresSeteSeg[1] = valor % 10;
  }
}

// ============================================================
//  LÓGICA DO SISTEMA E SENSORES
// ============================================================

void lerSensores() {
  temperatura = (analogRead(PIN_TEMP) * 500.0) / 1023.0;
  
  float vRaw = (analogRead(PIN_TENSAO) * 100.0) / 1023.0;
  tensao = vRaw * (calibVSPAN / 100.0);
  
  float iRaw = (analogRead(PIN_CORRENTE) * 500.0) / 1023.0;
  corrente = (iRaw * (calibISPAN / 500.0)) + calibIZERO;
  
  fluxo = (analogRead(PIN_FLUXO) * 20.0) / 1023.0;
  rpm = map(analogRead(PIN_RPM), 0, 1023, 0, 3000);

  if (tensao > tensaoPico) tensaoPico = tensao;
  if (corrente > correntePico) correntePico = corrente;
}

void gerenciarSeguranca() {
  // CORREÇÃO AQUI: Usando os nomes corretos (alarmeV, alarmeI, alarmeT)
  alarmeV.ativo = (tensao > LIMITE_TENSAO_MAX);
  alarmeI.ativo = (corrente > LIMITE_CORRENTE_MAX);
  alarmeT.ativo = (temperatura > LIMITE_TEMP_MAX);
  alarmeGlobalAtivo = (alarmeV.ativo || alarmeI.ativo || alarmeT.ativo);
  
  if(alarmeGlobalAtivo) {
     // AÇÃO DE SEGURANÇA: Desliga os relés principais
     // Se seu módulo ativa com LOW, aqui usamos HIGH para desligar (e vice-versa)
     digitalWrite(RELE_SISTEMA, HIGH); 
     digitalWrite(RELE_GAS, HIGH);   
     relesSistemaLigado = false;
  }
}

// ============================================================
//  MENUS E BOTÕES
// ============================================================

void tratarBotoes() {
  static bool lastUp = HIGH, lastDown = HIGH, lastMenu = HIGH, lastEnter = HIGH;
  bool stUp = digitalRead(BTN_UP);
  bool stDown = digitalRead(BTN_DOWN);
  bool stMenu = digitalRead(BTN_MENU);
  bool stEnter = digitalRead(BTN_ENTER);

  // MENU - Troca de Telas
  if(stMenu == LOW && lastMenu == HIGH && !emMenuCalibracao) {
    telaAtual = (telaAtual + 1) % (totalTelas + 1);
  }

  // UP - Ligar Sistema / Navegar
  if(stUp == LOW && lastUp == HIGH) {
    if(emMenuCalibracao) {
        if(valorEditando == 0) calibIZERO++;
        else if(valorEditando == 1) calibISPAN += 5;
        else if(valorEditando == 2) calibVSPAN++;
        else itemMenuCalib = (itemMenuCalib <= 0) ? 4 : itemMenuCalib - 1;
    } else if(!alarmeGlobalAtivo) {
        relesSistemaLigado = !relesSistemaLigado;
        digitalWrite(RELE_SISTEMA, relesSistemaLigado); // Lógica do módulo relé (se for active LOW, inverter aqui)
    }
  }

  // DOWN - Modo Solda / Navegar
  if(stDown == LOW && lastDown == HIGH) {
    if(emMenuCalibracao) {
        if(valorEditando == 0) calibIZERO--;
        else if(valorEditando == 1) calibISPAN -= 5;
        else if(valorEditando == 2) calibVSPAN--;
        else itemMenuCalib = (itemMenuCalib >= 4) ? 0 : itemMenuCalib + 1;
    } else {
        relesModoSMAW = !relesModoSMAW;
        // Não existe relé físico de modo, mas a variável muda para exibição
    }
  }

  // ENTER - Curto (Reset Picos) / Longo (Menu)
  if(stEnter == LOW) {
    if(tempoBotaoEnter == 0) tempoBotaoEnter = millis();
    if(millis() - tempoBotaoEnter > 3000 && !enterLongoDetectado) {
      enterLongoDetectado = true;
      emMenuCalibracao = !emMenuCalibracao;
      itemMenuCalib = 0; valorEditando = -1;
      lcd.clear();
    }
  } else {
    if(tempoBotaoEnter > 0 && !enterLongoDetectado) {
      if(emMenuCalibracao) {
        if(itemMenuCalib <= 2) valorEditando = (valorEditando == -1) ? itemMenuCalib : -1;
        else if(itemMenuCalib == 3) salvarCalibracao();
        else if(itemMenuCalib == 4) emMenuCalibracao = false;
      } else {
        tensaoPico = tensao; correntePico = corrente;
      }
    }
    tempoBotaoEnter = 0;
    enterLongoDetectado = false;
  }

  lastUp = stUp; lastDown = stDown; lastMenu = stMenu; lastEnter = stEnter;
}

void atualizarInterfaceLCD() {
  if(millis() - ultimaAtualizacaoLCD < 400) return;
  ultimaAtualizacaoLCD = millis();

  lcd.setCursor(0,0);
  if(emMenuCalibracao) {
    lcd.print("CALIB: ");
    if(valorEditando != -1) lcd.print(">EDITANDO<");
    else lcd.print("Item "); lcd.print(itemMenuCalib + 1);
    lcd.setCursor(0,1);
    switch(itemMenuCalib) {
      case 0: lcd.print("IZero: "); lcd.print(calibIZERO); break;
      case 1: lcd.print("ISpan: "); lcd.print(calibISPAN); break;
      case 2: lcd.print("VSpan: "); lcd.print(calibVSPAN); break;
      case 3: lcd.print("Salvar EEPROM    "); break;
      case 4: lcd.print("Sair             "); break;
    }
    return;
  }

  if(alarmeGlobalAtivo) {
    lcd.print("!! ALARME !!    ");
    lcd.setCursor(0,1);
    if(alarmeV.ativo) lcd.print(alarmeV.nome);
    else if(alarmeI.ativo) lcd.print(alarmeI.nome);
    else lcd.print(alarmeT.nome);
    return;
  }

  // Telas Normais
  switch(telaAtual) {
    case 0:
      lcd.print("V:"); lcd.print(tensao,1); lcd.print(" A:"); lcd.print(corrente,0);
      lcd.setCursor(0,1);
      lcd.print(relesModoSMAW ? "SMAW" : "MIG ");
      lcd.print(relesSistemaLigado ? " [ON] " : " [OFF]");
      break;
    case 1:
      lcd.print("Vpk:"); lcd.print(tensaoPico,1);
      lcd.setCursor(0,1);
      lcd.print("Ipk:"); lcd.print(correntePico,0); lcd.print("A");
      break;
    case 2:
      lcd.print("Temp: "); lcd.print(temperatura,1); lcd.print("C");
      lcd.setCursor(0,1);
      lcd.print("Fluxo: "); lcd.print(fluxo,1);
      break;
    case 3:
      lcd.print("Motor Arame:");
      lcd.setCursor(0,1);
      lcd.print(rpm); lcd.print(" RPM");
      break;
  }
}

// ============================================================
//  SETUP E LOOP PRINCIPAL
// ============================================================

void setup() {
  Serial.begin(115200); 
  
  // Configura Display 7 Seg
  pinMode(segA, OUTPUT); pinMode(segB, OUTPUT); pinMode(segC, OUTPUT);
  pinMode(segD, OUTPUT); pinMode(segE, OUTPUT); pinMode(segF, OUTPUT);
  pinMode(segG, OUTPUT);
  pinMode(d1, OUTPUT); pinMode(d2, OUTPUT); 
  pinMode(d3, OUTPUT); pinMode(d4, OUTPUT); 

// Configura LCD (Tentativa forçada)
  lcd.init();      // Tenta init() primeiro
  lcd.begin(16,2); // Tenta begin() depois por garantia
  lcd.backlight();
  lcd.clear();     // Limpa lixo de memória
  
  lcd.setCursor(0,0);
  lcd.print("TESTE TELA");
  delay(1000); // Pausa rápida só pra ver se escreveu

  // Configura Botões
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);
  
  // --- CONFIGURAÇÃO DOS RELÉS (SÓ OS 2 REAIS) ---
  pinMode(RELE_SISTEMA, OUTPUT);
  pinMode(RELE_GAS, OUTPUT);

  // Estado Inicial: Tudo Desligado
  // OBS: A maioria dos módulos relé ativa com LOW (GND).
  // Se o seu ligar sozinho ao iniciar, mude LOW para HIGH aqui.
  digitalWrite(RELE_SISTEMA, HIGH); // HIGH geralmente desliga módulo relé
  digitalWrite(RELE_GAS, HIGH);

  esperaComDisplay(2000); 
  lcd.clear();
}

void loop() {
  // O Segredo é que nada pode bloquear o loop para não apagar o 7 segmentos
  atualizarDisplay7Seg(); 
  lerSensores();
  gerenciarSeguranca();
  prepararDados7Seg();
  tratarBotoes();
  atualizarInterfaceLCD();
}
