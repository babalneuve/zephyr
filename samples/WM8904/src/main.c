/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i2c.h>

#define wm8904_I2C_NODE       DT_ALIAS(i3c)
#define wm8904_I2C_ADDR       0x1a
#define wm8904_I2C_ADDR_WRITE 0x34
#define wm8904_I2C_ADDR_READ  0x35

#define NUM_BLOCKS     20
#define SAMPLE_NO      64
#define FRAME_CLK_FREQ 48000
#define TIMEOUT        2000

/* The data_l represent a sine wave */
static int16_t data_l[SAMPLE_NO] = {
	3211,   6392,   9511,   12539,  15446,  18204,  20787,  23169,  25329,  27244,  28897,
	30272,  31356,  32137,  32609,  32767,  32609,  32137,  31356,  30272,  28897,  27244,
	25329,  23169,  20787,  18204,  15446,  12539,  9511,   6392,   3211,   0,      -3212,
	-6393,  -9512,  -12540, -15447, -18205, -20788, -23170, -25330, -27245, -28898, -30273,
	-31357, -32138, -32610, -32767, -32610, -32138, -31357, -30273, -28898, -27245, -25330,
	-23170, -20788, -18205, -15447, -12540, -9512,  -6393,  -3212,  -1,
};

/* The data_r represent a sine wave shifted by 90 deg to data_l sine wave */
static int16_t data_r[SAMPLE_NO] = {
	32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,  20787,  18204,  15446,
	12539,  9511,   6392,   3211,   0,      -3212,  -6393,  -9512,  -12540, -15447, -18205,
	-20788, -23170, -25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767, -32610,
	-32138, -31357, -30273, -28898, -27245, -25330, -23170, -20788, -18205, -15447, -12540,
	-9512,  -6393,  -3212,  -1,     3211,   6392,   9511,   12539,  15446,  18204,  20787,
	23169,  25329,  27244,  28897,  30272,  31356,  32137,  32609,  32767,
};

static const struct device *const i2c_dev = DEVICE_DT_GET(wm8904_I2C_NODE);

#define BLOCK_SIZE (2 * sizeof(data_l))

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

char MEM_SLAB_CACHE_ATTR
	__aligned(WB_UP(32)) _k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS+2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab,
			rx_0_mem_slab) = Z_MEM_SLAB_INITIALIZER(rx_0_mem_slab,
								_k_mem_slab_buf_rx_0_mem_slab,
								WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

char MEM_SLAB_CACHE_ATTR
	__aligned(WB_UP(32)) _k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS)*WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab,
			tx_0_mem_slab) = Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab,
								_k_mem_slab_buf_tx_0_mem_slab,
								WB_UP(BLOCK_SIZE), NUM_BLOCKS);

int wm8904_reg_write(const struct device *i2c_dev, uint8_t reg_addr, uint16_t value)
{
	uint8_t buf[3];

	buf[0] = reg_addr;
	buf[1] = value >> 8;   // Octet de poids fort de la valeur
	buf[2] = value & 0xFF; // Octet de poids faible de la valeur

	struct i2c_msg msgs[1];
	msgs[0].buf = (uint8_t *)buf;
	msgs[0].len = ARRAY_SIZE(buf);
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	if (i2c_transfer(i2c_dev, msgs, ARRAY_SIZE(msgs), wm8904_I2C_ADDR) != 0) {
		printk("Erreur lors de l'écriture du registre WM8904.\n");
		return -1;
	}

	return 0;
}

int wm8904_write_read(const struct device *i2c_dev, const void *write_buf, size_t num_write,
		      void *read_buf, size_t num_read)
{
	struct i2c_msg msg[2];

	msg[0].buf = (uint8_t *)write_buf;
	msg[0].len = num_write;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)read_buf;
	msg[1].len = num_read;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	if (i2c_transfer(i2c_dev, msg, ARRAY_SIZE(msg), wm8904_I2C_ADDR) != 0) {
		printk("Erreur i2c write read\n");
		return -1;
	}
	return 0;
}

bool init_wm8904_i2c(void)
{
	int ret;

	if (!device_is_ready(i2c_dev)) {
		printk("%s is not ready\n", i2c_dev->name);
		return false;
	} else {
		printk("%s is ready\n", i2c_dev->name);
	}

	uint8_t reg_test[1] = {0x44};
	uint8_t test[2];
	uint16_t united_test;

	static const uint16_t init[][2] = {
		{0x00, 0x0000}, // Réinitialisation générale (active tous les chemins, active les
				// horloges)
		{0x04, 0x0018}, {0x05, 0x0047}, {0x05, 0x0043}, {0x04, 0x0019}, {0x0e, 0x0003},
		{0x0f, 0x0003}, {0x16, 0x0002}, {0x12, 0x000c}, {0xff, 0x0000}, {0x04, 0x0019},
		{0x62, 0x0001}, {0xff, 0x0000}, {0x5a, 0x0011}, {0x5e, 0x0011}, {0x5a, 0x0033},
		{0x5e, 0x0033}, {0x43, 0x000f}, {0x44, 0x00f0}, {0xff, 0x0000}, {0x5a, 0x0077},
		{0x5e, 0x0077}, {0x5a, 0x00ff}, {0x5e, 0x00ff}, {0xff, 0x0000}, {0x19, 0x0012},
		{0xff, 0x0000}, {0xff, 0x0000}
	};

	for (int i = 0; i < ARRAY_SIZE(init); ++i) {
		const uint16_t *entry = init[i];

		ret = wm8904_reg_write(i2c_dev, (uint8_t)entry[0], entry[1]);

		if (ret < 0) {
			printk("Initialization step %d failed\n", i);
			return false;
		}

		k_msleep(100);
	}

	ret = wm8904_write_read(i2c_dev, reg_test, ARRAY_SIZE(reg_test), &test, ARRAY_SIZE(test));

	if (ret < 0) {
		printk("test error\n");
		return false;
	}

	united_test = (test[0] * 256) | test[1];

	printk("registre 0x44 --> 0x%x\n", united_test);
	return true;
}

int config_i2s(const struct device *dev_i2s_tx, const struct device *dev_i2s_rx)
{
	int ret;
	struct i2s_config i2s_cfg_tx, i2s_cfg_rx;

	i2s_cfg_tx.word_size = 16U;
	i2s_cfg_tx.channels = 2U;
	i2s_cfg_tx.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg_tx.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg_tx.block_size = BLOCK_SIZE;
	i2s_cfg_tx.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg_tx.mem_slab = &tx_0_mem_slab;
	i2s_cfg_tx.timeout = SYS_FOREVER_MS;

	i2s_cfg_rx.word_size = 16U;
	i2s_cfg_rx.channels = 2U;
	i2s_cfg_rx.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg_rx.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg_rx.block_size = BLOCK_SIZE;
	i2s_cfg_rx.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
	i2s_cfg_rx.mem_slab = &rx_0_mem_slab;
	i2s_cfg_tx.timeout = SYS_FOREVER_MS;

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

void fill_buf(int16_t *tx_block, int att)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = data_l[i] >> att;
		tx_block[2 * i + 1] = data_r[i] >> att;
	}
}

void check_buf(int16_t *rx_block){
	int sample_no = SAMPLE_NO;

	for (int i = 0; i < sample_no; i++) {
		printk("%d\n", rx_block[2*i]);
		printk("%d\n", rx_block[2*i+1]);
	}
}

int main()
{
	const struct device *const i2s_dev_rx = DEVICE_DT_GET(DT_NODELABEL(i2s0));
	const struct device *const i2s_dev_tx = DEVICE_DT_GET(DT_NODELABEL(i2s1));

	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int rx_idx = 0, cycle = 0, i, ret;
	//bool test = false;

	if (!init_wm8904_i2c()) {
		printk("Problème init\n");
		return 0;
	}

	printk("I2S sample\n");

	if (!device_is_ready(i2s_dev_tx)) {
		printk("%s is not ready\n", i2s_dev_tx->name);
		return 0;
	} else {
		printk("%s is ready\n", i2s_dev_tx->name);
	}

	if (!device_is_ready(i2s_dev_rx)) {
		printk("%s is not ready\n", i2s_dev_rx->name);
		return 0;
	} else {
		printk("%s is ready\n", i2s_dev_rx->name);
	}

	printk("I2S Init.\n");

	config_i2s(i2s_dev_tx, i2s_dev_rx);

	printk("I2S ready\n");

	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[i], K_FOREVER);
		if (ret < 0) {
			printk("Problème cycle n°%d : erreur allocation mémoire\n", cycle);
		}
		fill_buf(tx_block[i], i % 3);
	}

	i=0;

	ret = i2s_write(i2s_dev_tx, tx_block[i++], BLOCK_SIZE);
	if (ret < 0) {
		printk("Problème cycle n°%d : transmission buffer initiale\n", cycle);
		return ret;
	}

	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		printf("Problème cycle n°%d : could not trigger start I2S tx\n", cycle);
		return ret;
	}

	ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_START);
	if (ret < 0) {
		printf("Problème cycle n°%d : could not trigger start I2S rx\n", cycle);
		return ret;
	}

	while (1) {
		cycle++;

		for (; i < NUM_BLOCKS; i++) {
			ret = i2s_write(i2s_dev_tx, tx_block[i], BLOCK_SIZE);
			if (ret < 0) {
				printk("Problème cycle n°%d : transmission buffer n°%d\n", cycle, i);
				return ret;
			}

			ret = i2s_read(i2s_dev_rx, &rx_block[rx_idx], &rx_size);
			if (ret < 0) {
				printk("Problème cycle n°%d : réception buffer n°%d\n", cycle, rx_idx);
				return ret;
			}

			rx_idx++;
		}

		for (rx_idx = 0; rx_idx < NUM_BLOCKS; rx_idx++) {
			check_buf((uint16_t *)rx_block[rx_idx]);
			k_mem_slab_free(&rx_0_mem_slab, rx_block[rx_idx]);
		}

		ret = i2s_read(i2s_dev_rx, &rx_block[rx_idx], &rx_size);
		if (ret < 0) {
			printk("Problème cycle n°%d : réception buffer n°%d\n", cycle, rx_idx);
			return ret;
		}
		i=0;
		rx_idx = 0;

		printk("Cycle %i terminé\n",cycle);
	}

	ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_DROP);
	if (ret < 0) {
		printf("Problème cycle n°%d : could not trigger drain I2S tx\n", cycle);
		return ret;
	}

	ret = i2s_trigger(i2s_dev_rx, I2S_DIR_RX, I2S_TRIGGER_DROP);
	if (ret < 0) {
		printf("Problème cycle n°%d : could not trigger stop I2S rx\n", cycle);
		return ret;
	}

	printk("exit while\n");
	return 0;
}