/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>

#define GREEN_LED DT_ALIAS(led0)
#define BLUE_LED  DT_ALIAS(led1)
#define RED_LED   DT_ALIAS(led2)
#define BUTTON1   DT_ALIAS(sw0)
#define BUTTON2   DT_ALIAS(sw1)

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

#define MSG_SIZE 32

K_MSGQ_DEFINE(uart_queue, MSG_SIZE, 10, 4);

static const struct gpio_dt_spec ledg = GPIO_DT_SPEC_GET(GREEN_LED, gpios);
static const struct gpio_dt_spec ledb = GPIO_DT_SPEC_GET(BLUE_LED, gpios);
static const struct gpio_dt_spec ledr = GPIO_DT_SPEC_GET(RED_LED, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BUTTON1, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(BUTTON2, gpios);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE], tx_buf[MSG_SIZE];
static int rx_buf_pos;

K_THREAD_STACK_DEFINE(msg_thread_stack, 512);
K_THREAD_STACK_DEFINE(rblink_thread_stack, 256);
K_THREAD_STACK_DEFINE(bblink_thread_stack, 256);
K_THREAD_STACK_DEFINE(gblink_thread_stack, 256);

static struct k_thread msg_thread_data, rblink_thread_data, bblink_thread_data, gblink_thread_data;
k_tid_t thread_rblink, thread_bblink, thread_gblink;

struct k_sem rblink_sem, bblink_sem, gblink_sem;

void gpio_config()
{
	gpio_pin_configure_dt(&ledg, GPIO_OUTPUT);
	gpio_pin_configure_dt(&ledb, GPIO_OUTPUT);
	gpio_pin_configure_dt(&ledr, GPIO_OUTPUT);

	gpio_pin_configure_dt(&button1, GPIO_INPUT);
	gpio_pin_configure_dt(&button2, GPIO_INPUT);
}

int conv_char_int(char *chaine)
{
	int entier;
	char *endptr;

	entier = strtol(chaine, &endptr, 10);

	if (chaine == endptr) {
		printk("ERREUR : Aucun chiffre trouvé dans la chaîne.\n");
		return 0;
	} else if (*endptr != '\0') {
		printk("ERREUR : Caractère non numérique trouvé dans la chaîne.\n");
		return 0;
	} else {
		return entier;
	}
}

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		printk("ERREUR : Mise à jour configuration interruprions (IRQ)");
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		printk("ERREUR : Aucun message disponible");
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_queue, &rx_buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

void uart_config()
{
	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return;
	}
	uart_irq_rx_enable(uart_dev);
}

void rblink_thread(void *p1, void *p2, void *p3)
{
	int period;
	printk("-----------------------------------------------\n");
	printk("What Period do you want ? (in ms)\n");

	k_msgq_get(&uart_queue, &tx_buf, K_FOREVER);
	period = conv_char_int(tx_buf);

	printk("The red LED is blinking with a period of %i ms.\n", period);
	printk("-----------------------------------------------\n");

	k_sem_give(&rblink_sem);

	while (1) {
		gpio_pin_toggle_dt(&ledr);
		k_sleep(K_MSEC(period / 2));
	}
}

void bblink_thread(void *p1, void *p2, void *p3)
{
	int period;
	printk("-----------------------------------------------\n");
	printk("What Period do you want ? (in ms)\n");

	k_msgq_get(&uart_queue, &tx_buf, K_FOREVER);
	period = conv_char_int(tx_buf);

	printk("The red LED is blinking with a period of %i ms.\n", period);
	printk("-----------------------------------------------\n");

	k_sem_give(&bblink_sem);

	while (1) {
		gpio_pin_toggle_dt(&ledb);
		k_sleep(K_MSEC(period / 2));
	}
}

void gblink_thread(void *p1, void *p2, void *p3)
{
	int period;
	printk("-----------------------------------------------\n");
	printk("What Period do you want ? (in ms)\n");

	k_msgq_get(&uart_queue, &tx_buf, K_FOREVER);
	period = conv_char_int(tx_buf);

	printk("The red LED is blinking with a period of %i ms.\n", period);
	printk("-----------------------------------------------\n");

	k_sem_give(&gblink_sem);

	while (1) {
		gpio_pin_toggle_dt(&ledg);
		k_sleep(K_MSEC(period / 2));
	}
}

void choix_led(int8_t choix_mode)
{
	int8_t choix;
	bool back = 0;

	while (back == 0) {
		printk("-----------------------------------------------\n");

		switch (choix_mode) {
		case (1):
			printk("Tell me which led you want to blink :\n");
			break;
		case (2):
			printk("Tell me which led you want to switch ON :\n");
			break;
		case (3):
			printk("Tell me which led you want to switch OFF :\n");
		}

		printk("1-Red\n");
		printk("2-Blue\n");
		printk("3-Green\n");
		printk("4-Go back\n");

		k_msgq_get(&uart_queue, &tx_buf, K_FOREVER);
		choix = conv_char_int(tx_buf);

		switch (choix) {
		case (1):
			switch (choix_mode) {
			case (1):
				if (thread_rblink == NULL) {
					thread_rblink = k_thread_create(
						&rblink_thread_data, rblink_thread_stack,
						K_THREAD_STACK_SIZEOF(rblink_thread_stack),
						rblink_thread, NULL, NULL, NULL, K_PRIO_COOP(1), 0,
						K_NO_WAIT);
					k_sem_take(&rblink_sem, K_FOREVER);
				} else {
					printk("Red LED already blink !\n");
					printk("-----------------------------------------------\n");
				}
				break;
			case (2):
				k_thread_abort(thread_rblink);
				thread_rblink = NULL;
				gpio_pin_set_dt(&ledr, 1);
				printk("The red LED is ON.\n");
				printk("-----------------------------------------------\n");
				break;
			case (3):
				k_thread_abort(thread_rblink);
				thread_rblink = NULL;
				gpio_pin_set_dt(&ledr, 0);
				printk("The red LED is OFF.\n");
				printk("-----------------------------------------------\n");
			}
			back = 1;
			break;
		case (2):
			switch (choix_mode) {
			case (1):
				if (thread_bblink == NULL) {
					thread_bblink = k_thread_create(
						&bblink_thread_data, bblink_thread_stack,
						K_THREAD_STACK_SIZEOF(bblink_thread_stack),
						bblink_thread, NULL, NULL, NULL, K_PRIO_COOP(1), 0,
						K_NO_WAIT);
					k_sem_take(&bblink_sem, K_FOREVER);
				} else {
					printk("Blue LED already blink !\n");
					printk("-----------------------------------------------\n");
				}
				break;
			case (2):
				k_thread_abort(thread_bblink);
				thread_bblink = NULL;
				gpio_pin_set_dt(&ledb, 1);
				printk("The blue LED is ON.\n");
				printk("-----------------------------------------------\n");
				break;
			case (3):
				k_thread_abort(thread_bblink);
				thread_bblink = NULL;
				gpio_pin_set_dt(&ledb, 0);
				printk("The blue LED is OFF.\n");
				printk("-----------------------------------------------\n");
			}
			back = 1;
			break;
		case (3):
			switch (choix_mode) {
			case (1):
				if (thread_gblink == NULL) {
					thread_gblink = k_thread_create(
						&gblink_thread_data, gblink_thread_stack,
						K_THREAD_STACK_SIZEOF(gblink_thread_stack),
						gblink_thread, NULL, NULL, NULL, K_PRIO_COOP(1), 0,
						K_NO_WAIT);
					k_sem_take(&gblink_sem, K_FOREVER);
				} else {
					printk("Green LED already blink !\n");
					printk("-----------------------------------------------\n");
				}
				break;
			case (2):
				k_thread_abort(thread_gblink);
				thread_gblink = NULL;
				gpio_pin_set_dt(&ledg, 1);
				printk("The green LED is ON.\n");
				printk("-----------------------------------------------\n");
				break;
			case (3):
				k_thread_abort(thread_gblink);
				thread_gblink = NULL;
				gpio_pin_set_dt(&ledg, 0);
				printk("The green LED is OFF.\n");
				printk("-----------------------------------------------\n");
			}
			back = 1;
			break;
		case (4):
			back = 1;
			printk("-----------------------------------------------\n");
			break;
		default:
			printk("This choice is not available.\n");
			printk("-----------------------------------------------\n");
			break;
		}
	}
}

void msg_thread(void *p1, void *p2, void *p3)
{
	int8_t choix;

	while (1) {
		printk("-----------------------------------------------\n");
		printk("Hello ! Tell me what you want to do :\n");
		printk("1-Blink\n");
		printk("2-LED ON\n");
		printk("3-LED OFF\n");

		k_msgq_get(&uart_queue, &tx_buf, K_FOREVER);
		choix = conv_char_int(tx_buf);

		switch (choix) {
		case (0):
			break;
		case (1):
			printk("You chose Blink.\n");
			printk("-----------------------------------------------\n");
			choix_led(choix);
			break;
		case (2):
			printk("You chose LED ON.\n");
			printk("-----------------------------------------------\n");
			choix_led(choix);
			break;
		case (3):
			printk("You chose LED OFF.\n");
			printk("-----------------------------------------------\n");
			choix_led(choix);
			break;
		default:
			printk("This choice is not available.\n");
			printk("-----------------------------------------------\n");
		}
	}
}

int main()
{
	gpio_config();
	uart_config();

	k_sem_init(&rblink_sem, 0, 1);
	k_sem_init(&bblink_sem, 0, 1);
	k_sem_init(&gblink_sem, 0, 1);

	k_thread_create(&msg_thread_data, msg_thread_stack, K_THREAD_STACK_SIZEOF(msg_thread_stack),
			msg_thread, NULL, NULL, NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);

	while (1) {
	}
	return 0;
}