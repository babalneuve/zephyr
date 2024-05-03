/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#define LED1 DT_ALIAS(led0)
#define LED2 DT_ALIAS(led1)
#define BOUTON DT_ALIAS(sw0)

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2, gpios);
static const struct gpio_dt_spec bouton = GPIO_DT_SPEC_GET(BOUTON, gpios);

int main(void)
{	
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT);
	gpio_pin_configure_dt(&bouton, GPIO_INPUT);

	bool etat_bouton;

	while(1){
		etat_bouton = gpio_pin_get_dt(&bouton);
		if(etat_bouton == 1){
			gpio_pin_set_dt(&led2, 0);
			gpio_pin_toggle_dt(&led1);
			k_msleep(500);
			printk("led");
		}else{
			gpio_pin_set_dt(&led1, 0);
			gpio_pin_toggle_dt(&led2);
			k_msleep(500);
		}
	}
	return 0;
}

