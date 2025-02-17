#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define BTN_B_PIN 6

#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27
#define BLUE_LED_PIN 12
#define RED_LED_PIN 13
#define GREEN_LED_PIN 11
#define JOYSTICK_BUTTON_PIN 22

#define PIXEL_SIZE 8
#define PWM_WRAP 255

volatile bool button_pressed = false;
volatile bool green_led_state = false;
uint8_t pixel_x = (WIDTH - PIXEL_SIZE) / 2;
uint8_t pixel_y = (HEIGHT - PIXEL_SIZE) / 2;

void enter_bootsel()
{
  reset_usb_boot(0, 0);
}

void button_isr_handler(uint gpio, uint32_t events)
{
  if (gpio == BTN_B_PIN && events & GPIO_IRQ_EDGE_FALL)
  {
    button_pressed = true;
  }
}

void setup_button_interrupt()
{
  gpio_init(BTN_B_PIN);
  gpio_set_dir(BTN_B_PIN, GPIO_IN);
  gpio_pull_up(BTN_B_PIN);

  gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr_handler);
}

uint8_t map_adc_to_screen(uint16_t adc_value, uint8_t max_value)
{
  return (uint8_t)((adc_value * max_value) / 4095);
}

void setup_pwm_for_led(uint gpio)
{
  gpio_set_function(gpio, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(gpio);
  pwm_set_wrap(slice_num, PWM_WRAP);
  pwm_set_enabled(slice_num, true);
}

int main()
{
  stdio_init_all();
  adc_init();

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
  setup_pwm_for_led(BLUE_LED_PIN);
  setup_pwm_for_led(RED_LED_PIN);

  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN);

  gpio_init(GREEN_LED_PIN);
  gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);

  gpio_init(JOYSTICK_BUTTON_PIN);
  gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
  gpio_pull_up(JOYSTICK_BUTTON_PIN);

  uint16_t adc_value_x;
  uint16_t adc_value_y;

  while (true)
  {
    adc_select_input(1);
    adc_value_x = adc_read();
    adc_select_input(0);
    adc_value_y = adc_read();

    ssd1306_fill(&ssd, false);

    pixel_x = map_adc_to_screen(adc_value_x, WIDTH - PIXEL_SIZE);
    pixel_y = map_adc_to_screen(4095 - adc_value_y, HEIGHT - PIXEL_SIZE);

    ssd1306_rect(&ssd, pixel_y, pixel_x, PIXEL_SIZE, PIXEL_SIZE, true, true);
    ssd1306_send_data(&ssd);

    uint16_t blue_pwm_value = 0;
    uint16_t red_pwm_value = 0;

    if (adc_value_y > 2200)
    {
      blue_pwm_value = (uint16_t)((adc_value_y - 2200) / 2200.0 * 255);
    }
    else if (adc_value_y < 1800)
    {
      blue_pwm_value = (uint16_t)((1800 - adc_value_y) / 1800.0 * 255);
    }

    if (adc_value_x > 2200)
    {
      red_pwm_value = (uint16_t)((adc_value_x - 2200) / 2200.0 * 255);
    }
    else if (adc_value_x < 1800)
    {
      red_pwm_value = (uint16_t)((1800 - adc_value_x) / 1800.0 * 255);
    }

    pwm_set_gpio_level(BLUE_LED_PIN, blue_pwm_value);
    pwm_set_gpio_level(RED_LED_PIN, red_pwm_value);

    if (button_pressed)
    {
      button_pressed = false;
      enter_bootsel();
    }

    if (gpio_get(JOYSTICK_BUTTON_PIN) == 0)
    {
      green_led_state = !green_led_state;
      gpio_put(GREEN_LED_PIN, green_led_state);
      sleep_ms(300);
    }

    sleep_ms(100);
  }
}