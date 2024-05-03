/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
 
 //conteur qui change l'état de la led verte quand il atteint la valeur 10

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS	1000

/* The devicetree node identifier for the "led0" alias. */
#define GREEN_LED DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(GREEN_LED, gpios);

int main(void)
{
	int i=0;
	int ret;

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	while(1){
		i++;

		if(i>10){
			i=0;
			ret = gpio_pin_toggle_dt(&led);
		}

		printk("la valeur de i est de %i\n",i);
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}

