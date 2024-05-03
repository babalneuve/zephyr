/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#define SLEEP_TIME_MS	500		//Definition du temps de pause

/* The devicetree node identifier */
#define GREEN_LED DT_ALIAS(led0)
#define BLUE_LED DT_ALIAS(led1)
#define RED_LED DT_ALIAS(led2)
#define BOUTON1 DT_ALIAS(sw0)
#define BOUTON2 DT_ALIAS(sw1)

// Enregistrement des informations des entrées/sorties physiques
static const struct gpio_dt_spec ledg = GPIO_DT_SPEC_GET(GREEN_LED, gpios);
static const struct gpio_dt_spec ledb = GPIO_DT_SPEC_GET(BLUE_LED, gpios);
static const struct gpio_dt_spec ledr = GPIO_DT_SPEC_GET(RED_LED, gpios);
static const struct gpio_dt_spec bouton1 = GPIO_DT_SPEC_GET(BOUTON1, gpios);
static const struct gpio_dt_spec bouton2 = GPIO_DT_SPEC_GET(BOUTON2, gpios);

// Fonctions pour les interruptions
void button1_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    static bool led_state = false;

    led_state = !led_state;

    gpio_pin_set_dt(&ledb, led_state);
}

void button2_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    static bool led_state = false;

    led_state = !led_state;

    gpio_pin_set_dt(&ledg, led_state);
}

int main(void)
{	
	static struct gpio_callback button_cb1, button_cb2;
	
	// Configuration des entrées et sorties
	gpio_pin_configure_dt(&ledg, GPIO_OUTPUT);
	gpio_pin_configure_dt(&ledb, GPIO_OUTPUT);
	gpio_pin_configure_dt(&ledr, GPIO_OUTPUT);
	gpio_pin_configure_dt(&bouton1, GPIO_INPUT);
	gpio_pin_configure_dt(&bouton2, GPIO_INPUT);

	// Initialisation des conditions d'interruptions
	gpio_pin_interrupt_configure_dt(&bouton1, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&bouton2, GPIO_INT_EDGE_TO_ACTIVE);

	// Configuration des tâches des interruptions
	gpio_init_callback(&button_cb1, button1_pressed, BIT(bouton1.pin));
    gpio_add_callback(bouton1.port, &button_cb1);

	gpio_init_callback(&button_cb2, button2_pressed, BIT(bouton2.pin));
    gpio_add_callback(bouton2.port, &button_cb2);

	while(1){
		gpio_pin_toggle_dt(&ledr);	// Clignottement LED rouge
		k_msleep(SLEEP_TIME_MS);	// PAUSE
	}

	return 0;
}

