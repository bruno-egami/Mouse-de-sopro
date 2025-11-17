# Mouse de Sopro (Sip-and-Puff) com Transdutor Analógico

Este projeto consiste em um mouse de acessibilidade controlado por sopro e sucção (Sip-and-Puff) e um joystick analógico. O sistema utiliza um Arduino com capacidade HID nativa para emular um mouse USB padrão.

> **Atualização (17/11/2025):** O projeto migrou de sensores digitais BMP280 (I2C) para um transdutor de pressão analógico dedicado. Esta mudança simplificou o hardware, eliminou a necessidade de conversores de nível lógico e multiplexadores, e resolveu problemas de instabilidade na comunicação.

## 📋 Lista de Materiais (Hardware Atual)

* **Microcontrolador:** Arduino Leonardo ou Pro Micro (ATmega32U4).
* **Sensor de Pressão:** 1x Transdutor de Pressão Analógico (Range típico: -14.5 a 30 PSI ou similar, Saída: 0.5V ~ 4.5V).
* **Navegação:** 1x Módulo Joystick Analógico (KY-023 ou similar).
* **Conexão:** Cabos Jumper e Protoboard (ou PCB customizada).
* **Alimentação:** Via cabo USB (5V) conectado ao computador.

---

## ⚙️ Configuração e Pinos

O código foi simplificado para leitura analógica direta. Não são necessárias bibliotecas externas além da `Mouse.h` (padrão do Arduino).

### Mapa de Pinos (Pinout)

| Componente | Pino do Módulo | Pino no Arduino | Função |
| :--- | :--- | :--- | :--- |
| **Transdutor** | Sinal (Signal) | **A0** | Leitura da pressão (Sopro/Sucção) |
| **Joystick** | VRx (Horizontal) | **A1** | Movimento do cursor (Eixo X) |
| **Joystick** | VRy (Vertical) | **A2** | Movimento do cursor (Eixo Y) |
| **Alimentação**| VCC / GND | 5V / GND | Alimentação comum para todos os módulos |

---

## 🎮 Funcionalidades e Modos de Operação

O sistema calibra automaticamente a pressão atmosférica local ao iniciar (não sopre no tubo durante os primeiros 2 segundos ao ligar).

### Comandos de Sopro e Sucção

O software diferencia ações baseadas no tempo de duração do sopro/sucção (Limiar de tempo: 600ms).

1.  **Sopro Rápido:** Clique Esquerdo (Simples).
2.  **Suga Rápido:** Clique Direito.
3.  **Sopro Longo:** Clique Duplo Esquerdo (Double Click).
4.  **Suga Longo:** Ativa modo **Arrastar (Drag & Drop)**.
    * *O botão esquerdo "segura" o clique. Para soltar o item arrastado, basta realizar um sopro (curto ou longo).*

---

## 🛠️ Como Ajustar (Calibragem)

No código fonte, existem variáveis que podem ser alteradas para ajustar o conforto do usuário:

* `mouseSensitivity`: Aumente para o cursor mover mais rápido.
* `pressureThreshold`: Aumente se o mouse estiver clicando sozinho; diminua se for necessário muita força pulmonar.
* `longPressTime`: Tempo em milissegundos para diferenciar um clique curto de um comando longo.

---

## 📜 Histórico e Evolução do Projeto

### Versão Anterior (Descontinuada - I2C/BMP280)
Inicialmente, o projeto tentou utilizar módulos BMP280. Embora funcionais, apresentaram alta complexidade de hardware:
* **Materiais Antigos:** 3x BMP280, Multiplexer I2C, Level Shifter, Fonte Externa.
* **Problemas Enfrentados:** * Conflito de endereços I2C (exigia Multiplexer).
    * Incompatibilidade de tensão lógica 3.3V/5V (exigia Level Shifter).
    * Instabilidade e travamentos na comunicação I2C devido a interferências elétricas ao tocar nos fios.
    * Necessidade de fonte externa devido ao consumo e quedas de tensão.

**Solução:** A substituição pelo **Transdutor Analógico de 5V** eliminou todos os pontos de falha acima, resultando em um circuito mais robusto, mais barato e mais fácil de montar.

---

## 💻 Cabeçalho do Código Fonte

```cpp
/*
 ================================================================================
  Mouse de Sopro (Transdutor) 17/11/2025 - Funções de Modo e Correções
 ================================================================================
 Descrição Geral:
  Este código transforma um Arduino com capacidade USB nativa em um mouse,
  controlado por botões. 

  Compatibilidade:
  - Placas: Funciona em Arduinos com capacidade HID nativa, como Leonardo,
    Pro Micro e Due.
  - Bibliotecas: Requer as bibliotecas padrão "Mouse.h".
 ================================================================================ 
  Modos de Operação:
   
  1. Modo Mouse (Padrão):
      - Controle o cursor com o movimento do módulo analógico.
      - Botões de ação (clique esquerdo e clique direito):
        - Clique esquerdo: Aplicando pressão positiva (sopro) ao transdutor de pressão;
        - Clique direito: Aplicando pressão negativa (sugar) ao transdutor de pressão
   
 ================================================================================
    Pinos:
  --------------------------------------------------------------------------------
  BOTÕES DE AÇÃO:
  * Pino A0: Sinal do Transdutor
   
  ENCODERS (JOYSTICK):
  * Pino A1: Encoder Horizontal CLK
  * Pino A2: Encoder Horizontal DT
    ================================================================================
*/
