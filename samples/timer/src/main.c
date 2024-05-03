/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#define LED1 DT_ALIAS(led0)
#define LED2 DT_ALIAS(led1)
#define LED3 DT_ALIAS(led2)

#define BUTTON1 DT_ALIAS(sw0)
#define BUTTON2 DT_ALIAS(sw1)

#define BEGIN_DELAY K_MSEC(5000)
#define TOGGLE_DELAY K_MSEC(2000)

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1,gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2,gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3,gpios);

static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BUTTON1,gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(BUTTON2,gpios);

struct gpio_callback button_callback_data;

K_TIMER_DEFINE(blink_timer, NULL, NULL);

void gpio_configure(){
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT);
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT);

	gpio_pin_configure_dt(&button1, GPIO_INPUT);
	gpio_pin_configure_dt(&button2, GPIO_INPUT);

	gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&button2, GPIO_INT_EDGE_TO_ACTIVE);
}

void button_pressed(){
	k_timer_start(&blink_timer, BEGIN_DELAY, TOGGLE_DELAY);
}

void blink_leds(){
	printk("toggle1\n");
	gpio_pin_toggle_dt(&led1);
	printk("toggle2\n");
	gpio_pin_toggle_dt(&led2);
	printk("toggle3\n");
	gpio_pin_toggle_dt(&led3);
}

int main(void)
{	
	gpio_configure();

	k_timer_init(&blink_timer, blink_leds, NULL);

	gpio_init_callback(&button_callback_data, button_pressed, BIT(button1.pin)|BIT(button2.pin));
	gpio_add_callback_dt(&button1, &button_callback_data);
	gpio_add_callback_dt(&button2, &button_callback_data);

	while(1){
		k_sleep(K_MSEC(2000));
	}
	return 0;
}

