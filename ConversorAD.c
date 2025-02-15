#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "pico/bootrom.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define BUTTON_PIN 6

volatile bool button_pressed = false;

void enter_bootsel()
{
  reset_usb_boot(0, 0);
}

void button_isr_handler(uint gpio, uint32_t events)
{
  if (gpio == BUTTON_PIN && events & GPIO_IRQ_EDGE_FALL)
  {
    button_pressed = true;
  }
}

void setup_button_interrupt()
{
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN);

  gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr_handler);
}

int main()
{
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  ssd1306_t ssd;
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);

  ssd1306_fill(&ssd, false);

  uint8_t center_x = (WIDTH - 8) / 2;
  uint8_t center_y = (HEIGHT - 8) / 2;

  ssd1306_rect(&ssd, center_y, center_x, 8, 8, true, true);

  ssd1306_send_data(&ssd);

  setup_button_interrupt();

  while (true)
  {
    if (button_pressed)
    {
      button_pressed = false;
      enter_bootsel();
    }
  }
}