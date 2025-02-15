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

#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27

#define JOYSTICK_CENTER_MIN 1500
#define JOYSTICK_CENTER_MAX 2500

volatile bool button_pressed = false;
uint8_t pixel_x = (WIDTH - 8) / 2;
uint8_t pixel_y = (HEIGHT - 8) / 2;

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

void move_left()
{
  if (pixel_x > 0)
  {
    pixel_x--;
  }
}

void move_right()
{
  if (pixel_x < WIDTH - 8)
  {
    pixel_x++;
  }
}

void move_up()
{
  if (pixel_y < HEIGHT - 8)
  {
    pixel_y++;
  }
}

void move_down()
{
  if (pixel_y > 0)
  {
    pixel_y--;
  }
}

void reset_position_if_centered(uint16_t adc_value_x, uint16_t adc_value_y)
{
  if (adc_value_x >= JOYSTICK_CENTER_MIN && adc_value_x <= JOYSTICK_CENTER_MAX &&
      adc_value_y >= JOYSTICK_CENTER_MIN && adc_value_y <= JOYSTICK_CENTER_MAX)
  {
    pixel_x = (WIDTH - 8) / 2;
    pixel_y = (HEIGHT - 8) / 2;
  }
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
  ssd1306_send_data(&ssd);

  setup_button_interrupt();

  adc_init();
  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN);

  uint16_t adc_value_x;
  uint16_t adc_value_y;

  while (true)
  {
    adc_select_input(1);
    adc_value_x = adc_read();
    adc_select_input(0);
    adc_value_y = adc_read();

    ssd1306_fill(&ssd, false);

    if (adc_value_x < 1000)
    {
      move_left();
    }
    else if (adc_value_x > 3500)
    {
      move_right();
    }

    if (adc_value_y < 1000)
    {
      move_up();
    }
    else if (adc_value_y > 3500)
    {
      move_down();
    }

    reset_position_if_centered(adc_value_x, adc_value_y);

    ssd1306_rect(&ssd, pixel_y, pixel_x, 8, 8, true, true);
    ssd1306_send_data(&ssd);

    if (button_pressed)
    {
      button_pressed = false;
      enter_bootsel();
    }

    sleep_ms(100);
  }
}
