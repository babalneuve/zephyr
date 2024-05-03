/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#define LED DT_ALIAS(led1)
#define BOUTON DT_ALIAS(sw0)

#define THREAD_SIZE 1024

#define BLINK_THREAD_PRIORITY K_PRIO_COOP(5)
#define BUTTON_THREAD_PRIORITY K_PRIO_COOP(5)

#define BLINK_TIME K_MSEC(500)
#define TIMER_TIME K_MSEC(100)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED, gpios);
static const struct gpio_dt_spec bouton = GPIO_DT_SPEC_GET(BOUTON, gpios);

K_THREAD_STACK_DEFINE(blink_thread_stack, THREAD_SIZE);
struct k_thread blink_thread_data;

K_THREAD_STACK_DEFINE(button_thread_stack, THREAD_SIZE);
struct k_thread button_thread_data;

struct k_sem button_sem;

// Thread qui gère le clignottement habituel
void blink_thread(void *p1, void *p2, void *p3){
	while(1){
		k_sem_take(&button_sem, K_FOREVER);	//bloque le prog si la sémaphore n'est pas reçu
		gpio_pin_toggle_dt(&led);
		k_sleep(BLINK_TIME);
	}
}

// Thread qui gère si le bouton est appuyé ou non
void button_thread(void *p1, void *p2, void *p3){
	bool button_state;

	while(1){
		button_state = gpio_pin_get_dt(&bouton);

		if(button_state == 1){
			gpio_pin_toggle_dt(&led);
			k_sleep(TIMER_TIME);
		}else{
			k_sem_give(&button_sem);	//envoie la sémaphore
			k_sleep(K_MSEC(400));	//évite d'envoyer trop de sémaphore
		}

		k_sleep(K_MSEC(10));
	}
}

int main(void)
{	
	gpio_pin_configure_dt(&bouton, GPIO_INPUT);
	gpio_pin_configure_dt(&led, GPIO_OUTPUT);

	k_sem_init(&button_sem, 0, 1);	//initialise la sémaphore

	k_thread_create(&blink_thread_data, blink_thread_stack, K_THREAD_STACK_SIZEOF(blink_thread_stack),
					blink_thread, NULL, NULL, NULL, BLINK_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&button_thread_data, button_thread_stack, K_THREAD_STACK_SIZEOF(button_thread_stack),
					button_thread, NULL, NULL, NULL, BUTTON_THREAD_PRIORITY, 0, K_NO_WAIT);

	while (1){
	}
	
	return 0;
}

