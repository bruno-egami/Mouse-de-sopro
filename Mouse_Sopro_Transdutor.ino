/*
 ================================================================================
  PROJETO: Mouse de Sopro (Sip-and-Puff) Controlado por Transdutor
  DATA: 17/11/2025
  VERSÃO: Rev 4 (Comentada)
 ================================================================================
 DESCRIÇÃO:
  Este código transforma um Arduino (Leonardo/Pro Micro) em um mouse acessível.
  - O movimento é controlado por um joystick analógico.
  - Os cliques são controlados por um transdutor de pressão (Sopro e Sucção).

 COMPATIBILIDADE:
  - Arduino Leonardo, Pro Micro, Micro (Chip ATmega32U4).
  
 MODOS DE OPERAÇÃO (LÓGICA):
  1. Sopro Curto: Clique Esquerdo (Simples).
  2. Sopro Longo: Clique Duplo (Double Click).
  3. Suga Curto:  Trava/Destrava o Clique Esquerdo (Função Arrastar/Drag).
  4. Suga Longo:  Clique Direito.
 ================================================================================
*/

// Inclui a biblioteca nativa do Arduino para emular um Mouse USB
#include <Mouse.h>

// ==============================================================================
// 1. CONFIGURAÇÕES DE HARDWARE (PINOUT)
// ==============================================================================
// Define em quais pinos físicos os componentes estão conectados
const int pinTransducer = A0; // Pino do sinal do Transdutor de Pressão
const int pinJoyX = A1;       // Pino do eixo horizontal do Joystick
const int pinJoyY = A2;       // Pino do eixo vertical do Joystick
const int pinLed = 13;        // LED da placa (usado para indicar calibração)

// ==============================================================================
// 2. AJUSTES DE SENSIBILIDADE E PREFERÊNCIAS
// ==============================================================================
// Define a velocidade do cursor. Quanto maior o número, mais rápido o mouse move.
const int mouseSensitivity = 5;       

// Define a orientação do joystick. 
// Mude para 'true' ou 'false' caso o mouse esteja se movendo ao contrário.
const bool invertHorizontal = true;   
const bool invertVertical   = true;  

// Define a "força" necessária no sopro/sucção para ativar o clique.
// Valor entre 0 e 1023. Aumente se estiver clicando sozinho.
const int pressureThreshold = 30;        

// Define quanto tempo (em milissegundos) o usuário deve manter o sopro/sucção
// para que o Arduino entenda como um comando "Longo".
const unsigned long longPressTime = 600; 

// ==============================================================================
// 3. CONFIGURAÇÕES DE CALIBRAÇÃO (STARTUP)
// ==============================================================================
// Tempo que o Arduino espera ao ligar antes de começar a ler (para estabilizar a voltagem)
const unsigned long stabilizationTime = 2000; 
// Quantas leituras o Arduino fará para calcular a média do "zero" do sensor
const int calibrationSamples = 200;           

// ==============================================================================
// 4. VARIÁVEIS GLOBAIS (MEMÓRIA DO SISTEMA)
// ==============================================================================
int baselinePressure = 0;          // Guarda o valor de pressão ambiente (o "zero")
bool isDragging = false;           // Variável de controle: O botão esquerdo está preso?
unsigned long actionStartTime = 0; // Cronômetro: Marca quando o sopro começou
bool actionAlreadyExecuted = false;// Trava: Impede que uma ação se repita no mesmo sopro

// Enumeração: Cria nomes legíveis para os estados do sensor
enum State { 
  NEUTRAL, // Estado Neutro (Sem sopro nem sucção)
  PUFFING, // Estado de Sopro (Pressão Positiva)
  SIPPING  // Estado de Sucção (Pressão Negativa)
};

// Variáveis que armazenam o estado atual e o anterior para comparação
State currentState = NEUTRAL;
State lastState = NEUTRAL;

// ==============================================================================
// 5. SETUP (EXECUTADO UMA VEZ AO LIGAR)
// ==============================================================================
void setup() {
  // Inicia a comunicação serial (útil para debug no PC)
  Serial.begin(9600);
  
  // Configura o pino do LED como saída
  pinMode(pinLed, OUTPUT);
  
  // --- FASE 1: ESTABILIZAÇÃO ---
  Serial.println("Aguardando estabilizacao do hardware...");
  
  // Marca o tempo atual
  unsigned long startWait = millis();
  
  // Loop de espera: Fica aqui até dar o tempo de 'stabilizationTime'
  while (millis() - startWait < stabilizationTime) {
    // Pisca o LED para avisar o usuário: "Aguarde, não sopre agora"
    digitalWrite(pinLed, HIGH); delay(100);
    digitalWrite(pinLed, LOW);  delay(100);
  }

  // --- FASE 2: CALIBRAÇÃO ---
  Serial.println("Lendo linha de base (Zero)...");
  digitalWrite(pinLed, HIGH); // Acende o LED fixo (Indicando leitura)
  
  long total = 0; // Variável temporária para somar as leituras
  
  // Faz 200 leituras do sensor
  for (int i = 0; i < calibrationSamples; i++) {
    total += analogRead(pinTransducer);
    delay(5); // Pequena pausa para evitar ruído elétrico
  }
  
  // Calcula a média (Soma total dividida pelo número de leituras)
  baselinePressure = total / calibrationSamples;
  
  digitalWrite(pinLed, LOW); // Apaga o LED. O sistema está pronto.
  
  Serial.print("Calibracao concluida. Valor Base: ");
  Serial.println(baselinePressure);
  
  // Inicia a biblioteca de emulação do Mouse
  Mouse.begin();
}

// ==============================================================================
// 6. LOOP (EXECUTADO REPETIDAMENTE)
// ==============================================================================
void loop() {
  
  // ---------------------------------------------------------
  // BLOCO A: LEITURA E MOVIMENTO DO JOYSTICK
  // ---------------------------------------------------------
  int xVal = analogRead(pinJoyX); // Lê o eixo X (0 a 1023)
  int yVal = analogRead(pinJoyY); // Lê o eixo Y (0 a 1023)
  
  // Matemática de Movimento:
  // Subtrai 512 para que o centro seja 0 (varia de -512 a +512)
  // Divide pela sensibilidade para suavizar o movimento
  int moveX = (xVal - 512) / (512 / mouseSensitivity);
  int moveY = (yVal - 512) / (512 / mouseSensitivity); 

  // Verifica se precisa inverter a direção conforme configurado no início
  if (invertHorizontal) moveX = -moveX;
  if (invertVertical)   moveY = -moveY;

  // Só envia comando ao PC se houver movimento real (diferente de 0)
  if (moveX != 0 || moveY != 0) {
    Mouse.move(moveX, moveY, 0); // (X, Y, Scroll)
  }

  // ---------------------------------------------------------
  // BLOCO B: LEITURA DO TRANSDUTOR DE PRESSÃO
  // ---------------------------------------------------------
  int currentPressure = analogRead(pinTransducer); // Lê o valor atual
  
  // Calcula a diferença entre o valor atual e o valor calibrado (zero)
  int diff = currentPressure - baselinePressure;
  
  // Define o Estado Atual baseado na força do sopro/sucção
  if (diff > pressureThreshold) {
    currentState = PUFFING; // Pressão positiva acima do limite -> Soprando
  } else if (diff < -pressureThreshold) {
    currentState = SIPPING; // Pressão negativa abaixo do limite -> Sugando
  } else {
    currentState = NEUTRAL; // Dentro da zona morta -> Parado
  }

  // ---------------------------------------------------------
  // BLOCO C: MÁQUINA DE ESTADOS (LÓGICA DOS CLIQUES)
  // ---------------------------------------------------------

  // C.1 -> INÍCIO DA AÇÃO (O momento exato que começa a soprar/sugar)
  if (currentState != NEUTRAL && lastState == NEUTRAL) {
    actionStartTime = millis();    // Inicia o cronômetro
    actionAlreadyExecuted = false; // Reseta a trava de execução
  }

  // C.2 -> DURANTE A AÇÃO (Enquanto o usuário mantém o sopro/sucção)
  if (currentState != NEUTRAL && !actionAlreadyExecuted) {
    
    // Verifica se o tempo decorrido já passou do tempo de "Longo"
    if (millis() - actionStartTime > longPressTime) {
      
      // CASO 1: SOPRO LONGO DETECTADO
      if (currentState == PUFFING) { 
        // Executa Clique Duplo
        Mouse.click(MOUSE_LEFT); 
        delay(50); 
        Mouse.click(MOUSE_LEFT);
        
        actionAlreadyExecuted = true; // Trava para não repetir
        
        // Segurança: Se estava arrastando algo, o duplo clique solta o objeto
        if (isDragging) { isDragging = false; } 
      } 
      // CASO 2: SUGA LONGO DETECTADO
      else if (currentState == SIPPING) { 
        // Executa Clique Direito
        
        // Segurança: Se estava arrastando, solta antes de clicar com o direito
        if (isDragging) {
           Mouse.release(MOUSE_LEFT);
           isDragging = false;
        }
        Mouse.click(MOUSE_RIGHT);
        
        actionAlreadyExecuted = true; // Trava para não repetir
      }
    }
  }

  // C.3 -> FIM DA AÇÃO (O momento que o usuário para de soprar/sugar)
  // Aqui processamos as ações "Curtas/Rápidas", pois o usuário soltou antes do tempo longo
  if (currentState == NEUTRAL && lastState != NEUTRAL) {
    
    // Só entra aqui se a ação Longa NÃO tiver acontecido ainda
    if (!actionAlreadyExecuted) {
      
      // CASO 3: O USUÁRIO PAROU DE SOPRAR (SOPRO CURTO)
      if (lastState == PUFFING) {
        // Se estava no modo arrastar, o sopro serve para SOLTAR o item
        if (isDragging) {
           Mouse.release(MOUSE_LEFT);
           isDragging = false;
        } else {
           // Se não estava arrastando, é um clique esquerdo normal
           Mouse.click(MOUSE_LEFT);
        }
      } 
      // CASO 4: O USUÁRIO PAROU DE SUGAR (SUGA CURTO)
      else if (lastState == SIPPING) {
        // Alterna o modo Arrastar (Toggle)
        if (!isDragging) {
          Mouse.press(MOUSE_LEFT); // Segura o botão esquerdo
          isDragging = true;       // Atualiza a variável de controle
        } else {
          Mouse.release(MOUSE_LEFT); // Solta o botão esquerdo
          isDragging = false;        // Atualiza a variável de controle
        }
      }
    }
  }

  // Atualiza o estado anterior para ser usado na próxima volta do loop
  lastState = currentState;
  
  // Pequeno atraso para estabilidade do processador
  delay(10); 
}
