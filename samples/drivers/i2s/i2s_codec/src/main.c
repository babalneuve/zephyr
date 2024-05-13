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


#define SAMPLE_FREQUENCY    48000
#define SAMPLE_BIT_WIDTH    (16U)
#define BYTES_PER_SAMPLE    sizeof(int16_t)
#define NUMBER_OF_CHANNELS  2U
/* Such block length provides an echo with the delay of 100 ms. */
#define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      2
#define TIMEOUT             (2000U)

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT (INITIAL_BLOCKS + 2)
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

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
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = NUMBER_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = SAMPLE_FREQUENCY;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.mem_slab = &mem_slab;
	i2s_cfg.timeout = TIMEOUT;

	ret = i2s_configure(dev_i2s_tx, I2S_DIR_TX, &i2s_cfg);
	if (ret < 0) {
		printk("Failed to configure I2S stream\n");
		return ret;
	}

	ret = i2s_configure(dev_i2s_rx, I2S_DIR_RX, &i2s_cfg);
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
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
	struct audio_codec_cfg audio_cfg;

	printk("I2S sample\n");

	if (!device_is_ready(dev_i2s_rx)) {
		printk("%s is not ready\n", dev_i2s_rx->name);
		return 0;
	} else {
		printk("%s is ready\n", dev_i2s_rx->name);
	}

	printk("I2S Init.\n");

	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = SAMPLE_BIT_WIDTH;
	audio_cfg.dai_cfg.i2s.channels =  2;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_SLAVE;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = SAMPLE_FREQUENCY;
	audio_cfg.dai_cfg.i2s.mem_slab = &mem_slab;
	audio_cfg.dai_cfg.i2s.block_size = BLOCK_SIZE;
	audio_codec_configure(codec_dev, &audio_cfg);
	k_msleep(1000);

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