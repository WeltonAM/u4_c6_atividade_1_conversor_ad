#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"  // Biblioteca principal do RP2040
#include "hardware/adc.h" // Biblioteca para ADC (Conversor Analógico-Digital)
#include "hardware/pwm.h" // Biblioteca para controle PWM (Modulação por Largura de Pulso)
#include "hardware/i2c.h" // Biblioteca para comunicação I2C (para o display SSD1306)
#include "lib/ssd1306.h"  // Biblioteca para o controle do display SSD1306
#include "lib/font.h"     // Biblioteca de fontes para o display SSD1306
#include "pico/bootrom.h" // Biblioteca para funções de boot do RP2040

// Definição dos pinos de hardware
#define I2C_PORT i2c1     // Usando I2C1
#define I2C_SDA 14        // Pino SDA para I2C
#define I2C_SCL 15        // Pino SCL para I2C
#define endereco 0x3C     // Endereço do display SSD1306
#define JOYSTICK_X_PIN 26 // Pino do eixo X do joystick
#define JOYSTICK_Y_PIN 27 // Pino do eixo Y do joystick
#define JOYSTICK_PB 22    // Pino do botão do joystick
#define Botao_A 5         // Pino do botão A
#define Botao_B 6         // Pino do botão B
#define LED_R 11          // Pino do LED vermelho
#define LED_G 12          // Pino do LED verde
#define LED_B 13          // Pino do LED azul
#define DEBOUNCE_DELAY 50 // Tempo de debounce (50 ms)

// Variáveis globais
ssd1306_t ssd;                           // Estrutura para o display SSD1306
bool led_pwm_ativo = true;               // Flag que indica se os LEDs PWM estão ativos
bool led_verde_estado = false;           // Estado do LED verde
int borda_estilo = 0;                    // Estilo da borda do display (0, 1, 2)
static uint32_t last_interrupt_time = 0; // Último tempo de interrupção

// Função para inicializar PWM em um pino
uint pwm_init_gpio(uint gpio, uint wrap)
{
  gpio_set_function(gpio, GPIO_FUNC_PWM);       // Configura o pino para PWM
  uint slice_num = pwm_gpio_to_slice_num(gpio); // Obtém o número do slice do pino
  pwm_set_wrap(slice_num, wrap);                // Define o valor máximo do contador PWM (4095)
  pwm_set_enabled(slice_num, true);             // Habilita o PWM
  return slice_num;                             // Retorna o número do slice
}

// Função de interrupção para o botão do joystick
void joystick_button_irq(uint gpio, uint32_t events)
{
  uint32_t current_time = to_ms_since_boot(get_absolute_time());
  if (current_time - last_interrupt_time > DEBOUNCE_DELAY)
  {
    led_verde_estado = !led_verde_estado;  // Alterna o estado do LED verde
    borda_estilo = (borda_estilo + 1) % 3; // Altera o estilo da borda do display
    last_interrupt_time = current_time;
  }
}

// Função de interrupção para o botão A
void botao_a_irq(uint gpio, uint32_t events)
{
  led_pwm_ativo = !led_pwm_ativo; // Alterna a ativação do PWM dos LEDs
}

// Função de interrupção para o botão B
void botao_b_irq(uint gpio, uint32_t events)
{
  ssd1306_fill(&ssd, false); // Limpa o display SSD1306
  ssd1306_send_data(&ssd);   // Atualiza o display
  reset_usb_boot(0, 0);      // Reinicia o dispositivo
}

// Função principal
int main()
{
  stdio_init_all(); // Inicializa as funções de entrada e saída

  adc_init();                    // Inicializa o ADC para leitura dos eixos do joystick
  adc_gpio_init(JOYSTICK_X_PIN); // Inicializa o pino X do joystick
  adc_gpio_init(JOYSTICK_Y_PIN); // Inicializa o pino Y do joystick

  uint pwm_wrap = 4095; // Valor máximo de PWM (4095, resolução de 12 bits)

  // Inicializa os LEDs para controle PWM
  pwm_init_gpio(LED_R, pwm_wrap);
  pwm_init_gpio(LED_G, pwm_wrap);
  pwm_init_gpio(LED_B, pwm_wrap);

  // Inicializa o barramento I2C para comunicação com o display
  i2c_init(I2C_PORT, 400 * 1000);            // Configura o I2C com uma taxa de 400kHz
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura o pino SDA
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura o pino SCL
  gpio_pull_up(I2C_SDA);                     // Habilita o pull-up no pino SDA
  gpio_pull_up(I2C_SCL);                     // Habilita o pull-up no pino SCL

  // Inicializa o display SSD1306
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);    // Configura o display
  ssd1306_send_data(&ssd); // Atualiza o display

  // Configuração dos botões para interrupções
  gpio_init(JOYSTICK_PB);
  gpio_set_dir(JOYSTICK_PB, GPIO_IN);                                                              // Configura o botão do joystick como entrada
  gpio_pull_up(JOYSTICK_PB);                                                                       // Habilita o pull-up no pino do botão
  gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, &joystick_button_irq); // Interrupção no botão

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);                                                      // Configura o botão A como entrada
  gpio_pull_up(Botao_A);                                                               // Habilita o pull-up no pino do botão A
  gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &botao_a_irq); // Interrupção no botão A

  gpio_init(Botao_B);
  gpio_set_dir(Botao_B, GPIO_IN);                                                      // Configura o botão B como entrada
  gpio_pull_up(Botao_B);                                                               // Habilita o pull-up no pino do botão B
  gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &botao_b_irq); // Interrupção no botão B

  while (true)
  {
    adc_select_input(0);         // Seleciona o canal do eixo X do joystick
    uint16_t adc_x = adc_read(); // Lê o valor do eixo X do joystick
    adc_select_input(1);         // Seleciona o canal do eixo Y do joystick
    uint16_t adc_y = adc_read(); // Lê o valor do eixo Y do joystick

    // Controle do PWM dos LEDs com base nos valores do joystick
    if (led_pwm_ativo)
    {
      // Ajusta o brilho do LED vermelho com base no valor do eixo X
      pwm_set_gpio_level(LED_R, abs((int)adc_x - 2048) * 255 / 2048);
      // Ajusta o brilho do LED azul com base no valor do eixo Y
      pwm_set_gpio_level(LED_B, abs((int)adc_y - 2048) * 255 / 2048);
    }
    else
    {
      pwm_set_gpio_level(LED_R, 0); // Desliga o LED vermelho
      pwm_set_gpio_level(LED_B, 0); // Desliga o LED azul
    }

    // Controle do LED verde
    pwm_set_gpio_level(LED_G, led_verde_estado ? pwm_wrap : 0);

    // Limpa o display e desenha o quadrado
    ssd1306_fill(&ssd, false);
    if (borda_estilo == 1)
    {
      ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); // Borda 1 (total)
    }
    else if (borda_estilo == 2)
    {
      ssd1306_rect(&ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false); // Borda 2 (pequena)
    }

    // Desenha um quadrado de 8x8 pixels baseado nos valores do joystick
    ssd1306_rect(&ssd, 60 + (adc_x * 128 / 4095) - 4, 32 + (adc_y * 64 / 4095) - 4, 8, 8, true, true);
    ssd1306_send_data(&ssd); // Atualiza o display

    sleep_ms(100); // Aguarda 100ms antes de repetir o loop
  }
}