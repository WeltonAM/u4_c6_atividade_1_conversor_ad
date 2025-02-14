#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "pico/bootrom.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27
#define JOYSTICK_PB 22
#define Botao_A 5
#define Botao_B 6
#define LED_R 11
#define LED_G 12
#define LED_B 13

ssd1306_t ssd;
bool led_pwm_ativo = true;
bool led_verde_estado = false;
int borda_estilo = 0;

uint pwm_init_gpio(uint gpio, uint wrap)
{
  gpio_set_function(gpio, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(gpio);
  pwm_set_wrap(slice_num, wrap);
  pwm_set_enabled(slice_num, true);
  return slice_num;
}

void joystick_button_irq(uint gpio, uint32_t events)
{
  led_verde_estado = !led_verde_estado;
  borda_estilo = (borda_estilo + 1) % 3;
}

void botao_a_irq(uint gpio, uint32_t events)
{
  led_pwm_ativo = !led_pwm_ativo;
}

void botao_b_irq(uint gpio, uint32_t events)
{
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);
  reset_usb_boot(0, 0);
}

int main()
{
  stdio_init_all();
  adc_init();
  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN);

  uint pwm_wrap = 4095;
  pwm_init_gpio(LED_R, pwm_wrap);
  pwm_init_gpio(LED_G, pwm_wrap);
  pwm_init_gpio(LED_B, pwm_wrap);

  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  gpio_init(JOYSTICK_PB);
  gpio_set_dir(JOYSTICK_PB, GPIO_IN);
  gpio_pull_up(JOYSTICK_PB);
  gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, &joystick_button_irq);

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);
  gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &botao_a_irq);

  gpio_init(Botao_B);
  gpio_set_dir(Botao_B, GPIO_IN);
  gpio_pull_up(Botao_B);
  gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &botao_b_irq);

  while (true)
  {
    adc_select_input(0);
    uint16_t adc_x = adc_read();
    adc_select_input(1);
    uint16_t adc_y = adc_read();

    if (led_pwm_ativo)
    {
      pwm_set_gpio_level(LED_R, abs((int)adc_x - 2048) * 255 / 2048);
      pwm_set_gpio_level(LED_B, abs((int)adc_y - 2048) * 255 / 2048);
    }
    else
    {
      pwm_set_gpio_level(LED_R, 0);
      pwm_set_gpio_level(LED_B, 0);
    }

    pwm_set_gpio_level(LED_G, led_verde_estado ? pwm_wrap : 0);

    ssd1306_fill(&ssd, false);
    if (borda_estilo == 1)
    {
      ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false);
    }
    else if (borda_estilo == 2)
    {
      ssd1306_rect(&ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false);
    }

    ssd1306_rect(&ssd, 60 + (adc_x * 128 / 4095) - 4, 32 + (adc_y * 64 / 4095) - 4, 8, 8, true, true);
    ssd1306_send_data(&ssd);

    sleep_ms(100);
  }
}