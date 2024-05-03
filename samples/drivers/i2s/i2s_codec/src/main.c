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

int config_i2s(const struct device *dev_i2s_rx) {
	int ret;
	struct i2s_config i2s_cfg_rx;

	i2s_cfg_rx.word_size = 16U;
	i2s_cfg_rx.channels = NUMBER_OF_CHANNELS;
	i2s_cfg_rx.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg_rx.frame_clk_freq = SAMPLE_FREQUENCY;
	i2s_cfg_rx.block_size = BLOCK_SIZE;
	i2s_cfg_rx.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg_rx.mem_slab = &mem_slab;
	i2s_cfg_rx.timeout = TIMEOUT;

	ret = i2s_configure(dev_i2s_rx, I2S_DIR_RX, &i2s_cfg_rx);
	if (ret < 0) {
		printk("Failed to configure I2S stream\n");
		return ret;
	}
	return 0;
}

int main(void)
{
	const struct device *const i2s_dev_codec = DEVICE_DT_GET(I2S_CODEC_TX);
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
	struct audio_codec_cfg audio_cfg;
	int ret;

	printk("codec sample\n");

	if (!device_is_ready(i2s_dev_codec)) {
		printk("%s is not ready\n", i2s_dev_codec->name);
		return 0;
	}


	if (!device_is_ready(codec_dev)) {
		printk("%s is not ready", codec_dev->name);
		return 0;
	}
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = SAMPLE_BIT_WIDTH;
	audio_cfg.dai_cfg.i2s.channels =  2;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = SAMPLE_FREQUENCY;
	audio_cfg.dai_cfg.i2s.mem_slab = &mem_slab;
	audio_cfg.dai_cfg.i2s.block_size = BLOCK_SIZE;
	audio_codec_configure(codec_dev, &audio_cfg);
	k_msleep(1000);

	printk("I2S Init.\n");

	config_i2s(i2s_dev_codec);

	printk("I2S ready\n");

	printk("start streams\n");
	while (1) {
		int error = 0;
		/* Start reception */
		ret = i2s_trigger(i2s_dev_codec, I2S_DIR_RX, I2S_TRIGGER_START);
		if (ret < 0) {
			printf("Problème: could not trigger start I2S rx\n");
			return ret;
		}

		while (1) {
			void *rx_block;
			size_t rx_size;

			ret = i2s_read(i2s_dev_codec, &rx_block, &rx_size);
			if (ret < 0) {
				error++;
				printk("Problème (%i): réception buffer (%i)\n", ret, error);
			}else{
				error = 0;
				printk("Info reçu\n");
			}
			if (error == 1){
				printk("To many error in a row !\n");
				return error;
			}
		}

		/* Stop reception */
		ret = i2s_trigger(i2s_dev_codec, I2S_DIR_RX, I2S_TRIGGER_STOP);
		if (ret < 0) {
			printf("Problème: could not trigger stop I2S rx\n");
			return ret;
		}
	}
}