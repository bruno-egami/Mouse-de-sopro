/*
 ================================================================================
  Mouse de Sopro (Transdutor) 17/11/2025 - Funções de Modo e Correções
 ================================================================================
 Descrição Geral:
  Este código transforma um Arduino com capacidade USB nativa em um mouse,
  controlado por botões e sopro. 

  Compatibilidade:
  - Placas: Arduino Leonardo, Pro Micro, Due.
  - Bibliotecas: "Mouse.h".
 ================================================================================ 
  Modos de Operação:
   
  1. Modo Mouse (Joystick):
      - Controle do cursor via módulo analógico.
      
  2. Ações de Sopro e Sucção (Transdutor):
      - Sopro Rápido: Clique Esquerdo
      - Sopro Longo:  Clique Duplo Esquerdo
      - Suga Rápido:  Clique Direito
      - Suga Longo:   Segura Clique Esquerdo (Arrastar) -> Sopro para soltar
   
 ================================================================================
    Pinos:
  * A0: Transdutor de Pressão
  * A1: Eixo X (Horizontal)
  * A2: Eixo Y (Vertical)
 ================================================================================
*/

#include <Mouse.h>

// --- Configurações de Hardware ---
const int pinTransducer = A0;
const int pinJoyX = A1;
const int pinJoyY = A2;

// --- Configurações de Movimento e Sentido ---
const int mouseSensitivity = 5;       // Velocidade do cursor (maior = mais rápido)

// INVERSÃO DOS EIXOS:
// Mude para 'true' para inverter o sentido, ou 'false' para manter normal.
const bool invertHorizontal = true;   // Inverter Esquerda/Direita?
const bool invertVertical   = false;  // Inverter Cima/Baixo?

// --- Configurações de Pressão e Tempo ---
const int pressureThreshold = 30;     // Força necessária para ativar (0-1023)
const unsigned long longPressTime = 600; // Tempo (ms) para considerar "Longo"

// --- Variáveis de Estado ---
int baselinePressure = 0;      
bool isDragging = false;       
unsigned long actionStartTime = 0; 
bool actionAlreadyExecuted = false; 

enum State {
  NEUTRAL,
  PUFFING, // Soprando
  SIPPING  // Sugando
};

State currentState = NEUTRAL;
State lastState = NEUTRAL;

void setup() {
  Serial.begin(9600);
  Mouse.begin();

  // --- Calibração Inicial ---
  long total = 0;
  for (int i = 0; i < 100; i++) {
    total += analogRead(pinTransducer);
    delay(5);
  }
  baselinePressure = total / 100;
}

void loop() {
  // ==========================================
  // 1. Movimento do Mouse (Joystick)
  // ==========================================
  int xVal = analogRead(pinJoyX);
  int yVal = analogRead(pinJoyY);
  
  // Calcula o movimento (Centralizado em 512)
  int moveX = (xVal - 512) / (512 / mouseSensitivity);
  int moveY = (yVal - 512) / (512 / mouseSensitivity); 

  // Aplica a inversão de eixos conforme configuração
  if (invertHorizontal) moveX = -moveX;
  if (invertVertical)   moveY = -moveY;

  // Move se houver deslocamento
  if (moveX != 0 || moveY != 0) {
    Mouse.move(moveX, moveY, 0); 
  }

  // ==========================================
  // 2. Leitura de Pressão e Definição de Estado
  // ==========================================
  int currentPressure = analogRead(pinTransducer);
  int diff = currentPressure - baselinePressure;
  
  if (diff > pressureThreshold) {
    currentState = PUFFING;
  } else if (diff < -pressureThreshold) {
    currentState = SIPPING;
  } else {
    currentState = NEUTRAL;
  }

  // ==========================================
  // 3. Máquina de Estados de Ação
  // ==========================================

  // --- INÍCIO DA AÇÃO ---
  if (currentState != NEUTRAL && lastState == NEUTRAL) {
    actionStartTime = millis();
    actionAlreadyExecuted = false; 
    
    if (currentState == PUFFING && isDragging) {
      Mouse.release(MOUSE_LEFT);
      isDragging = false;
      actionAlreadyExecuted = true; 
    }
  }

  // --- DURANTE A AÇÃO ---
  if (currentState != NEUTRAL && !actionAlreadyExecuted) {
    unsigned long duration = millis() - actionStartTime;

    if (duration > longPressTime) {
      if (currentState == PUFFING) {
        Mouse.click(MOUSE_LEFT);
        delay(50);
        Mouse.click(MOUSE_LEFT);
        actionAlreadyExecuted = true; 
      } 
      else if (currentState == SIPPING) {
        if (!isDragging) {
          Mouse.press(MOUSE_LEFT);
          isDragging = true;
          actionAlreadyExecuted = true; 
        }
      }
    }
  }

  // --- FIM DA AÇÃO ---
  if (currentState == NEUTRAL && lastState != NEUTRAL) {
    if (!actionAlreadyExecuted) {
      if (lastState == PUFFING) {
        Mouse.click(MOUSE_LEFT);
      } 
      else if (lastState == SIPPING) {
        Mouse.click(MOUSE_RIGHT);
      }
    }
  }

  lastState = currentState;
  delay(10); 
}
