/*
 * include/linux/synaptics_s7301.h
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SYNAPTICS_TS_H_
#define _LINUX_SYNAPTICS_TS_H_

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#define SYNAPTICS_TS_NAME "synaptics_ts"

#define TS_MAX_X_COORD			1279
#define TS_MAX_Y_COORD			799
#define TS_MAX_Z_TOUCH			255
#define TS_MAX_W_TOUCH			100

#define REG_INTERRUPT_STATUS		0x14
#define REG_FINGER_STATUS		0X15
#define REG_RESET			0x85
#define REG_POINT_INFO			0X18

#define TS_READ_REGS_LEN		66
#define MAX_TOUCH_NUM			10
#define I2C_RETRY_CNT			5

enum {
	TS_STATE_INACTIVE = -1,
	TS_STATE_RELEASE,
	TS_STATE_PRESS,
	TS_STATE_MOVE,
};

struct touch_info {
	int strength;
	int width_major;
	int width_minor;
	int posX;
	int posY;
	int status;
};

struct synaptics_platform_data {
	int max_x;
	int max_y;
	int max_pressure;
	int max_width;
	void (*set_power)(bool);
	int (*mux_i2c_set)(int);
};

struct synaptics_ts_data {
	struct i2c_client	*client;
	struct input_dev	*input;
	struct synaptics_platform_data *pdata;
	struct touch_info touch_info[MAX_TOUCH_NUM];
	struct mutex	mutex;
#if CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
};

#endif	/* _LINUX_SYNAPTICS_TS_H_ */
