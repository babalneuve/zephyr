/*
 * Copyright (c) 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include <string.h>

#include "wm8904.h"


#define I2S_CODEC_TX  DT_ALIAS(i2s_codec_tx)

#define SAMPLE_FREQUENCY    CONFIG_SAMPLE_FREQ
#define SAMPLE_BIT_WIDTH    (16U)
#define BYTES_PER_SAMPLE    sizeof(int16_t)
#define NUMBER_OF_CHANNELS  2U
/* Such block length provides an echo with the delay of 100 ms. */
#define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      CONFIG_I2S_INIT_BUFFERS
#define TIMEOUT             (2000U)

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT (INITIAL_BLOCKS + 32)
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

bool init_wm8904_i2c(void) {
	int ret;

	if (!device_is_ready(i2c_dev)) {
		printk("%s is not ready\n", i2c_dev->name);
		return false;
	} else {
		printk("%s is ready\n", i2c_dev->name);
	}

	uint8_t reg_test[1] = {0x00};
	uint8_t test[2];
	uint16_t united_test;

	ret = wm8904_reg_read(reg_test, ARRAY_SIZE(reg_test), &test, ARRAY_SIZE(test));
	if (ret < 0) {
		printk("WM8904 is not ready\n");
		return false;
	}
	united_test = (test[0] * 256) | test[1];
	printk("registre 0x%x --> 0x%x\n", reg_test[0], united_test);

	ret = wm8904_reg_write(WM8904_SW_RESET_AND_ID, 0x0000);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_CLOCK_RATES_2, 0x000f);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_WRITE_SEQUENCER_0, 0x0100);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_WRITE_SEQUENCER_3, 0x0100);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_WaitOnWriteSequencer();
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_CLOCK_RATES_0, 0xa45f);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_POWER_MANAGEMENT_0, 0x0003);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_POWER_MANAGEMENT_2, 0x0003);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_POWER_MANAGEMENT_6, 0x000f);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_ADC_0, 0x0001);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_AUDIO_INTERFACE_0, 0x0050);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_DAC_DIGITAL_1, 0x0040);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_LEFT_INPUT_0, 0x0005);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_RIGHT_INPUT_0, 0x0005);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_OUT1_LEFT, 0x00ad);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_OUT1_RIGHT, 0x00ad);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_DC_SERVO_0, 0x0003);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_HP_0, 0x00ff);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_CLASS_W_0, 0x0001);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_CHARGE_PUMP_0, 0x0001);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_AUDIO_INTERFACE_1, 0x000a);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_CLOCK_RATES_2, 0x0000);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_CLOCK_RATES_1, 0x0c05);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_AUDIO_INTERFACE_1, 0x0002);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_CLOCK_RATES_2, 0x1007);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_CLOCK_RATES_2, 0x1007);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_AUDIO_INTERFACE_1, 0x0002);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_AUDIO_INTERFACE_3, 0x0040);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_LEFT_INPUT_1, 0x0040);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_RIGHT_INPUT_1, 0x0040);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_LEFT_INPUT_1, 0x0054);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_RIGHT_INPUT_1, 0x0054);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_OUT12_ZC, 0x0000);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_OUT1_LEFT, 0x00b9);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}

	ret = wm8904_reg_write(WM8904_ANALOGUE_OUT1_RIGHT, 0x00b9);
	if (ret < 0) {
		printk("WM8904 init reg error (%i)\n", ret);
		return false;
	}
	k_msleep(100);

	return true;
}

void read_buf(int16_t *rx_block) {
	int sample_no = BLOCK_SIZE;

	for (int i = 0; i < sample_no; i++) {
		printk("Data n°%d: %d\n", i, rx_block[i]);
	}
}

static bool prepare_transfer(const struct device *i2s_dev_rx,
			     const struct device *i2s_dev_tx)
{
	int ret;

	for (int i = 0; i < INITIAL_BLOCKS; ++i) {
		void *mem_block;

		ret = k_mem_slab_alloc(&mem_slab, &mem_block, K_NO_WAIT);
		if (ret < 0) {
			printk("Failed to allocate TX block %d: %d\n", i, ret);
			return false;
		}

		memset(mem_block, 0, BLOCK_SIZE);

		ret = i2s_write(i2s_dev_tx, mem_block, BLOCK_SIZE);
		if (ret < 0) {
			printk("Failed to write block for trigger\n");
			return false;
		}
	}

	return true;
}

int config_i2s(const struct device *dev_i2s_tx, const struct device *dev_i2s_rx)
{
	int ret;
	struct i2s_config i2s_cfg_tx, i2s_cfg_rx;

	i2s_cfg_tx.word_size = 16U;
	i2s_cfg_tx.channels = NUMBER_OF_CHANNELS;
	i2s_cfg_tx.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg_tx.frame_clk_freq = SAMPLE_FREQUENCY;
	i2s_cfg_tx.block_size = BLOCK_SIZE;
	i2s_cfg_tx.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg_tx.mem_slab = &mem_slab;
	i2s_cfg_tx.timeout = TIMEOUT;

	i2s_cfg_rx.word_size = 16U;
	i2s_cfg_rx.channels = NUMBER_OF_CHANNELS;
	i2s_cfg_rx.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg_rx.frame_clk_freq = SAMPLE_FREQUENCY;
	i2s_cfg_rx.block_size = BLOCK_SIZE;
	i2s_cfg_rx.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg_rx.mem_slab = &mem_slab;
	i2s_cfg_rx.timeout = TIMEOUT;

	ret = i2s_configure(dev_i2s_tx, I2S_DIR_TX, &i2s_cfg_tx);
	if (ret < 0) {
		printk("Failed to configure I2S stream\n");
		return ret;
	}

	ret = i2s_configure(dev_i2s_rx, I2S_DIR_RX, &i2s_cfg_rx);
	if (ret < 0) {
		printk("Failed to configure I2S stream\n");
		return ret;
	}
	return 0;
}

int main(void)
{
	const struct device *const dev_i2s_rx = DEVICE_DT_GET(DT_NODELABEL(i2s0));
	const struct device *const dev_i2s_tx = DEVICE_DT_GET(DT_NODELABEL(i2s1));

	if (!init_wm8904_i2c()) {
		printk("Problème init\n");
		return 0;
	}

	printk("I2S sample\n");

	if (!device_is_ready(dev_i2s_rx)) {
		printk("%s is not ready\n", dev_i2s_rx->name);
		return 0;
	} else {
		printk("%s is ready\n", dev_i2s_rx->name);
	}

	printk("I2S Init.\n");

	config_i2s(dev_i2s_tx, dev_i2s_rx);

	printk("I2S ready\n");

	int ret;

	while (1) {
		int error = 0;
		/* Start reception */

		if (!prepare_transfer(dev_i2s_rx, dev_i2s_tx)) {
			return 0;
		}

		ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_START);
		if (ret < 0) {
			printf("Problème: could not trigger start I2S rx\n");
			return ret;
		}

		ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_START);
		if (ret < 0) {
			printf("Problème: could not trigger start I2S rx\n");
			return ret;
		}

		while (1) {
			void *rx_block;
			uint32_t rx_size;

			ret = i2s_read(dev_i2s_rx, &rx_block, &rx_size);
			if (ret < 0) {
				error++;
				printk("Problème (%i): réception buffer (%i)\n", ret, error);
				break;
			}

			/* read_buf((uint16_t *)rx_block); */

			ret = i2s_write(dev_i2s_tx, rx_block, rx_size);
			if (ret < 0) {
				printk("Failed to write data: %d\n", ret);
				break;
			}
		}

		/* Stop reception */
		ret = i2s_trigger(dev_i2s_rx, I2S_DIR_RX, I2S_TRIGGER_DROP);
		if (ret < 0) {
			printf("Problème: could not trigger stop I2S rx\n");
			return ret;
		}

		ret = i2s_trigger(dev_i2s_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
		if (ret < 0) {
			printf("Problème: could not trigger stop I2S rx\n");
			return ret;
		}
	}
	return 0;
}