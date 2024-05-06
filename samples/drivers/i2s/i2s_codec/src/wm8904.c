/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wm8904.h"

int wm8904_reg_write(uint8_t reg_addr, uint16_t value)
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

int wm8904_reg_read(const void *write_buf, size_t num_write, void *read_buf, size_t num_read)
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

int wm8904_WaitOnWriteSequencer()
{
    int result;
    uint8_t value[2];
	uint8_t reg[1] = {WM8904_WRITE_SEQUENCER_4};
	uint16_t reg_val;

	result = wm8904_reg_read(reg, ARRAY_SIZE(reg), &value, ARRAY_SIZE(value));
	reg_val = (value[0] * 256) | value[1];

    while ((reg_val & 0x0001) != 0x0000){
		k_msleep(10);
        result = wm8904_reg_read(reg, ARRAY_SIZE(reg), &value, ARRAY_SIZE(value));
		reg_val = (value[0] * 256) | value[1];
    }

    return result;
}

int wm8904_reg_modify(uint8_t registre, uint16_t mask, uint16_t value){
	int ret;
	uint8_t reg[1] = {registre};
	uint8_t regVal[2];
	uint16_t val;

	ret=wm8904_reg_read(reg, ARRAY_SIZE(reg), &regVal, ARRAY_SIZE(regVal));
	if(ret<0){
		return ret;
	}

	val = (regVal[0] * 256) | regVal[1];
	val &= (uint16_t)~mask;
	val |= value;

	return wm8904_reg_write(reg[0], val);
}

int wm8904_read_allregisters(){
	uint8_t allRegisters[] = {
    0x00, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x0C, 0x0E, 0x0F, 0x12, 0x14, 0x15, 0x16, 0x18, 0x19, 0x1A, 0x1B,
    0x1E, 0x1F, 0x20, 0x21, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x39,
    0x3A, 0x3B, 0x3C, 0x3D, 0x43, 0x44, 0x45, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x5A, 0x5E, 0x62,
    0x68, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93,
    0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0xC6, 0xF7, 0xF8};

	int ret;

	for(int i = 0; i<ARRAY_SIZE(allRegisters); i++){
		uint8_t reg[1] = {allRegisters[i]};
		uint8_t test[2];
		uint16_t united_test;

		ret = wm8904_reg_read(reg, ARRAY_SIZE(reg), &test, ARRAY_SIZE(test));
		if (ret < 0) {
			printk("WM8904 is not ready\n");
			return ret;
		}
		united_test = (test[0] * 256) | test[1];
		printk("registre 0x%x --> 0x%x\n", allRegisters[i], united_test);
	}

	return 0;
}