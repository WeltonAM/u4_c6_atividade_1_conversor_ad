#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27
#define JOYSTICK_PB 22
#define Botao_A 5
#define Botao_B 6
#include "pico/bootrom.h"

ssd1306_t ssd;

void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

int main()
{
  gpio_init(Botao_B);
  gpio_set_dir(Botao_B, GPIO_IN);
  gpio_pull_up(Botao_B);
  gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  gpio_init(JOYSTICK_PB);
  gpio_set_dir(JOYSTICK_PB, GPIO_IN);
  gpio_pull_up(JOYSTICK_PB);

  gpio_init(Botao_A);
  gpio_set_dir(Botao_A, GPIO_IN);
  gpio_pull_up(Botao_A);

  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN);

  uint16_t adc_value_x;
  uint16_t adc_value_y;
  char str_x[5];
  char str_y[5];

  bool cor = true;
  while (true)
  {
    adc_select_input(0);
    adc_value_x = adc_read();
    adc_select_input(1);
    adc_value_y = adc_read();
    sprintf(str_x, "%d", adc_value_x);
    sprintf(str_y, "%d", adc_value_y);

    ssd1306_fill(&ssd, !cor);
    ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);
    ssd1306_line(&ssd, 3, 25, 123, 25, cor);
    ssd1306_line(&ssd, 3, 37, 123, 37, cor);
    ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6);
    ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);
    ssd1306_draw_string(&ssd, "ADC   JOYSTICK", 10, 28);
    ssd1306_draw_string(&ssd, "X    Y    PB", 20, 41);
    ssd1306_line(&ssd, 44, 37, 44, 60, cor);
    ssd1306_draw_string(&ssd, str_x, 8, 52);
    ssd1306_line(&ssd, 84, 37, 84, 60, cor);
    ssd1306_draw_string(&ssd, str_y, 49, 52);
    ssd1306_rect(&ssd, 52, 90, 8, 8, cor, !gpio_get(JOYSTICK_PB));
    ssd1306_rect(&ssd, 52, 102, 8, 8, cor, !gpio_get(Botao_A));
    ssd1306_rect(&ssd, 52, 114, 8, 8, cor, !cor);
    ssd1306_send_data(&ssd);

    sleep_ms(100);
  }
}