/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#define SLEEP_TIME_VERTE_MS	K_MSEC(500)		//Definition du temps de pause
#define SLEEP_TIME_BLEUE_MS	K_SECONDS(1)		//Definition du temps de pause

/* The devicetree node identifier */
#define GREEN_LED DT_ALIAS(led0)
#define BLUE_LED DT_ALIAS(led1)

// Enregistrement des informations des entrées/sorties physiques
static const struct gpio_dt_spec ledg = GPIO_DT_SPEC_GET(GREEN_LED, gpios);
static const struct gpio_dt_spec ledb = GPIO_DT_SPEC_GET(BLUE_LED, gpios);

// Déclaration de l'espace mémoire  des threads
K_THREAD_STACK_DEFINE(thread1_stack,512);
static struct k_thread thread1_data;

K_THREAD_STACK_DEFINE(thread2_stack,512);
static struct k_thread thread2_data;

struct led_info{
	const struct device *port;
	gpio_pin_t pin;
	k_timeout_t temps;
};

void led_blink(void *p1, void *p2, void *p3){
	struct led_info *led = (struct led_info *)p1;

	while(1){
		gpio_pin_set(led->port, led->pin, 1);
        k_sleep(led->temps);
        gpio_pin_set(led->port, led->pin, 0);
        k_sleep(led->temps);
	}
}

void main(void)
{	
	struct led_info led_bleue, led_verte;

	led_bleue.port = ledb.port;
	led_bleue.pin = ledb.pin;
	led_bleue.temps = SLEEP_TIME_BLEUE_MS;

	led_verte.port = ledg.port;
	led_verte.pin = ledg.pin;
	led_verte.temps = SLEEP_TIME_VERTE_MS;

	// Configuration des entrées et sorties
	gpio_pin_configure_dt(&ledg, GPIO_OUTPUT);
	gpio_pin_configure_dt(&ledb, GPIO_OUTPUT);

	// Création des threads
	k_thread_create(&thread1_data, thread1_stack, K_THREAD_STACK_SIZEOF(thread1_stack),
									  led_blink, &led_bleue, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);

	k_thread_create(&thread2_data, thread2_stack, K_THREAD_STACK_SIZEOF(thread2_stack),
									  led_blink, &led_verte, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);

	// Boucle while obligatoire sinon le programme se coupe
	while(1){

	}

}

