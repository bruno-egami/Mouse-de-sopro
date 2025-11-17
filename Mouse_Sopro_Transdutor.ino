/*
 ================================================================================
  Mouse de Sopro (Transdutor) 17/11/2025 - Funções de Modo e Correções
 ================================================================================
 Descrição Geral:
  Este código transforma um Arduino com capacidade USB nativa em um mouse,
  controlado por botões e sopro. 

  Compatibilidade:
  - Placas: Funciona em Arduinos com capacidade HID nativa, como Leonardo,
    Pro Micro e Due.
  - Bibliotecas: Requer as bibliotecas padrão "Mouse.h".
 ================================================================================ 
  Modos de Operação (Baseado em Tempo):
   
  1. Modo Mouse (Joystick):
      - Controle o cursor com o movimento do módulo analógico.
      
  2. Ações de Sopro e Sucção (Transdutor):
      - Sopro Rápido: Clique Esquerdo
      - Sopro Longo:  Clique Duplo Esquerdo
      - Suga Rápido:  Clique Direito
      - Suga Longo:   Segura Clique Esquerdo (Arrastar) -> Sopro para soltar
   
 ================================================================================
    Pinos:
  --------------------------------------------------------------------------------
  TRANSDUTOR:
  * Pino A0: Sinal do Transdutor de Pressão (0.5V - 4.5V)
   
  JOYSTICK (Eixos):
  * Pino A1: Eixo X (Horizontal)
  * Pino A2: Eixo Y (Vertical)
 ================================================================================
*/

#include <Mouse.h>

// --- Configurações de Hardware ---
const int pinTransducer = A0;
const int pinJoyX = A1;
const int pinJoyY = A2;

// --- Configurações de Sensibilidade e Tempo ---
const int mouseSensitivity = 5;       // Velocidade do cursor
const int pressureThreshold = 30;     // Força necessária para ativar (0-1023)
const unsigned long longPressTime = 600; // Tempo (ms) para considerar "Longo"

// --- Variáveis de Estado ---
int baselinePressure = 0;      // Valor de repouso do sensor
bool isDragging = false;       // Estado do modo arrastar
unsigned long actionStartTime = 0; // Marca quando o sopro/sucção começou
bool actionAlreadyExecuted = false; // Impede repetição de ação durante o mesmo sopro

// Enumeração para facilitar a leitura do estado atual
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
  // O sensor precisa estar em "repouso" ao ligar o Arduino
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

  // Move se houver deslocamento significativo (Deadzone implícito na divisão)
  if (moveX != 0 || moveY != 0) {
    Mouse.move(-moveX, moveY, 0); // Eixo X invertido para orientação comum
  }

  // ==========================================
  // 2. Leitura de Pressão e Definição de Estado
  // ==========================================
  int currentPressure = analogRead(pinTransducer);
  int diff = currentPressure - baselinePressure;
  
  // Determina o estado atual baseado nos limiares
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

  // --- INÍCIO DA AÇÃO (Borda de subida) ---
  if (currentState != NEUTRAL && lastState == NEUTRAL) {
    actionStartTime = millis();
    actionAlreadyExecuted = false; // Reseta flag para nova ação
    
    // REGRA DE SEGURANÇA: Se estiver arrastando e soprar, solta imediatamente
    if (currentState == PUFFING && isDragging) {
      Mouse.release(MOUSE_LEFT);
      isDragging = false;
      actionAlreadyExecuted = true; // Impede que este sopro gere um clique extra
    }
  }

  // --- DURANTE A AÇÃO (Mantendo sopro ou sucção) ---
  if (currentState != NEUTRAL && !actionAlreadyExecuted) {
    unsigned long duration = millis() - actionStartTime;

    // Checa se atingiu o tempo de "Longo"
    if (duration > longPressTime) {
      
      if (currentState == PUFFING) {
        // SOPRO LONGO -> Clique Duplo
        Mouse.click(MOUSE_LEFT);
        delay(50);
        Mouse.click(MOUSE_LEFT);
        actionAlreadyExecuted = true; // Marca como feito
      } 
      else if (currentState == SIPPING) {
        // SUGA LONGO -> Segura Clique Esquerdo (Arrastar)
        if (!isDragging) {
          Mouse.press(MOUSE_LEFT);
          isDragging = true;
          actionAlreadyExecuted = true; // Marca como feito
        }
      }
    }
  }

  // --- FIM DA AÇÃO (Borda de descida / Soltou) ---
  if (currentState == NEUTRAL && lastState != NEUTRAL) {
    unsigned long duration = millis() - actionStartTime;

    // Se a ação longa NÃO foi executada, significa que foi uma ação CURTA
    if (!actionAlreadyExecuted) {
      
      if (lastState == PUFFING) {
        // SOPRO CURTO -> Clique Esquerdo
        Mouse.click(MOUSE_LEFT);
      } 
      else if (lastState == SIPPING) {
        // SUGA CURTO -> Clique Direito
        Mouse.click(MOUSE_RIGHT);
      }
    }
  }

  // Atualiza o estado anterior para o próximo loop
  lastState = currentState;
  
  delay(10); // Estabilidade
}
