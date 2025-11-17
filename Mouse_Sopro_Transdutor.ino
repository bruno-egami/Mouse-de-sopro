/*
 ================================================================================
  Mouse de Sopro (Transdutor) 17/11/2025 - Rev 4: Novas Funções de Suga
 ================================================================================
 Descrição Geral:
  Mouse de sopro (Sip-and-Puff) com joystick, utilizando transdutor analógico.
  
  Modos de Operação:
  1. Joystick: Movimento do cursor (Eixos X/Y).
  2. Transdutor (Lógica Atualizada): 
     - Sopro Curto: Clique Esquerdo
     - Sopro Longo: Clique Duplo Esquerdo
     - Suga Curto:  Trava/Destrava Clique Esquerdo (Arrastar)
     - Suga Longo:  Clique Direito

 ================================================================================
    Pinos:
  * A0: Transdutor de Pressão
  * A1: Eixo X (Horizontal)
  * A2: Eixo Y (Vertical)
  * 13: LED de Status (Pisca na calibração)
 ================================================================================
*/

#include <Mouse.h>

// --- Configurações de Hardware ---
const int pinTransducer = A0;
const int pinJoyX = A1;
const int pinJoyY = A2;
const int pinLed = 13; 

// --- Configurações de Movimento e Sentido ---
const int mouseSensitivity = 5;       
const bool invertHorizontal = true;   
const bool invertVertical   = false;  

// --- Configurações de Pressão e Tempo ---
const int pressureThreshold = 30;        // Força para ativar
const unsigned long longPressTime = 600; // Tempo para ação longa (ms)

// --- CONFIGURAÇÕES DE CALIBRAÇÃO ---
const unsigned long stabilizationTime = 2000; 
const int calibrationSamples = 200;           

// --- Variáveis de Estado ---
int baselinePressure = 0;      
bool isDragging = false;       // Estado do modo arrastar (Botão esquerdo preso)
unsigned long actionStartTime = 0; 
bool actionAlreadyExecuted = false; 

enum State { NEUTRAL, PUFFING, SIPPING };
State currentState = NEUTRAL;
State lastState = NEUTRAL;

void setup() {
  Serial.begin(9600);
  pinMode(pinLed, OUTPUT);
  
  // -- FASE 1: ESTABILIZAÇÃO --
  Serial.println("Aguardando estabilizacao...");
  unsigned long startWait = millis();
  while (millis() - startWait < stabilizationTime) {
    digitalWrite(pinLed, HIGH); delay(100);
    digitalWrite(pinLed, LOW);  delay(100);
  }

  // -- FASE 2: CALIBRAÇÃO --
  Serial.println("Calibrando...");
  digitalWrite(pinLed, HIGH); 
  long total = 0;
  for (int i = 0; i < calibrationSamples; i++) {
    total += analogRead(pinTransducer);
    delay(5); 
  }
  baselinePressure = total / calibrationSamples;
  digitalWrite(pinLed, LOW); 
  
  Serial.print("Base: ");
  Serial.println(baselinePressure);
  
  Mouse.begin();
}

void loop() {
  // ==========================================
  // 1. Movimento do Mouse (Joystick)
  // ==========================================
  int xVal = analogRead(pinJoyX);
  int yVal = analogRead(pinJoyY);
  
  int moveX = (xVal - 512) / (512 / mouseSensitivity);
  int moveY = (yVal - 512) / (512 / mouseSensitivity); 

  if (invertHorizontal) moveX = -moveX;
  if (invertVertical)   moveY = -moveY;

  if (moveX != 0 || moveY != 0) {
    Mouse.move(moveX, moveY, 0); 
  }

  // ==========================================
  // 2. Leitura de Pressão
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
  // 3. Lógica de Ações (Atualizada)
  // ==========================================

  // --- INÍCIO (Borda de Subida) ---
  if (currentState != NEUTRAL && lastState == NEUTRAL) {
    actionStartTime = millis();
    actionAlreadyExecuted = false; 
  }

  // --- DURANTE (Mantendo sopro/sucção) ---
  if (currentState != NEUTRAL && !actionAlreadyExecuted) {
    if (millis() - actionStartTime > longPressTime) {
      
      if (currentState == PUFFING) { 
        // SOPRO LONGO -> Clique Duplo
        Mouse.click(MOUSE_LEFT); delay(50); Mouse.click(MOUSE_LEFT);
        actionAlreadyExecuted = true; 
        // Se estava arrastando, o duplo clique também solta o botão, resetamos o estado:
        if (isDragging) { isDragging = false; } 
      } 
      else if (currentState == SIPPING) { 
        // SUGA LONGO -> Clique Direito
        // Se estiver arrastando, solta antes de clicar direito para evitar confusão
        if (isDragging) {
           Mouse.release(MOUSE_LEFT);
           isDragging = false;
        }
        Mouse.click(MOUSE_RIGHT);
        actionAlreadyExecuted = true; 
      }
    }
  }

  // --- FIM (Soltou - Borda de Descida) ---
  // Aqui processamos as ações "Curtas" (Rápido)
  if (currentState == NEUTRAL && lastState != NEUTRAL) {
    
    // Só executa se não tiver feito a ação longa
    if (!actionAlreadyExecuted) {
      
      if (lastState == PUFFING) {
        // SOPRO RÁPIDO -> Clique Esquerdo
        if (isDragging) {
           // Se estava arrastando e soprou rápido, solta o item.
           Mouse.release(MOUSE_LEFT);
           isDragging = false;
        } else {
           Mouse.click(MOUSE_LEFT);
        }
      } 
      else if (lastState == SIPPING) {
        // SUGA RÁPIDO -> Alternar Arrastar (Toggle Drag)
        if (!isDragging) {
          Mouse.press(MOUSE_LEFT); // Segura
          isDragging = true;
        } else {
          Mouse.release(MOUSE_LEFT); // Solta
          isDragging = false;
        }
      }
    }
  }

  lastState = currentState;
  delay(10); 
}
