/*
 * Fs16xx device driver implementation.
 * Copyright (C) 2017 FourSemi Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/module.h>

#include "fs16xx_regs.h"
#include "fs16xx_calibration.h"
#include "fs16xx_preset.h"
#include "fs16xx.h"
#define I2C_RETRY_DELAY	 5 /* ms */

static DEFINE_MUTEX(lr_lock);
/*static int stereo_mode; */
static int chip_type = -1;
#define I2C_RETRIES	20

extern int fs16xx_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id);
extern int tfa98xx_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id);
extern int fs16xx_i2c_remove(struct i2c_client *client);
extern int tfa98xx_i2c_remove(struct i2c_client *i2c);

enum Smartpa_Chiptype {
	type_tfa = 0,
	type_foursemi = 1
};

static ssize_t smartpa_get_ic_type(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int smartpa_type;

	printk("%s: get smartpa type enter ,find ID\n", __func__);

	if (chip_type == type_foursemi) {
	smartpa_type = 1;
	printk("%s: smartpa foursemi device found\n", __func__);
	} else if (chip_type == type_tfa) {
		smartpa_type = 0;
	printk("%s: smartpa TFA device found\n", __func__);
	} else{
		smartpa_type = -1;
	printk("%s: invalid smartpa type\n", __func__);
		}


	printk("%s: smartpa type = %d,exit\n", __func__, smartpa_type);

	return scnprintf(buf, PAGE_SIZE, "%u\n", smartpa_type);
}


static DEVICE_ATTR(smartpa_ictype, S_IRUGO,
	 smartpa_get_ic_type, NULL);

static struct attribute *smartpa_attributes[] = {
	&dev_attr_smartpa_ictype.attr,
	NULL
};
static const struct attribute_group smartpa_attr_group = {
	.attrs = smartpa_attributes,
};


static int smartpa_i2c_read(struct i2c_client *client,
		u8 reg, u8 *value, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = {
	{
	 .addr = client->addr,
	 .flags = 0,
	 .len = 1,
	 .buf = &reg,
	 },
	{
	 .addr = client->addr,
	 .flags = I2C_M_RD,
	 .len = len,
	 .buf = value,
	 },
	};

	do {
	err = i2c_transfer(client->adapter, msgs,
				ARRAY_SIZE(msgs));
	if (err != ARRAY_SIZE(msgs))
		msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != ARRAY_SIZE(msgs)) && (++tries < I2C_RETRIES));

	if (err != ARRAY_SIZE(msgs)) {
	dev_err(&client->dev, "read transfer error %d\n"
		, err);
	err = -EIO;
	} else {
	err = 0;
	}

	return err;
}

/*static int smartpa_i2c_write(struct i2c_client *client,
		u8 *value, u8 len)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = {
	{
	 .addr = client->addr,
	 .flags = 0,
	 .len = len,
	 .buf = value,
	 },
	};

	do {
	err = i2c_transfer(client->adapter, msgs,
			ARRAY_SIZE(msgs));
	if (err != ARRAY_SIZE(msgs))
		msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != ARRAY_SIZE(msgs)) && (++tries < I2C_RETRIES));

	if (err != ARRAY_SIZE(msgs)) {
	dev_err(&client->dev, "write transfer error\n");
	err = -EIO;
	} else {
	err = 0;
	}

	return err;
}*/


static int smartpa_i2c_probe(struct i2c_client *i2c,
			  const struct i2c_device_id *id)
{
	int ret = -1;
	/*int i; */
	u8 id_val[2] = {0};

	dev_err(&i2c->dev, "%s:enter i2c addr=0x%x\n", __func__, i2c->addr);

	ret = smartpa_i2c_read(i2c, 0x03, id_val, 2);
	dev_err(&i2c->dev, "smartpa id = %d\n", id_val[0]);
	dev_err(&i2c->dev, "smartpa id = %d\n", id_val[1]);
	if (ret != 0 && id_val[0] == 0) {
		dev_err(&i2c->dev, "%s:i2c transfer error ret = %d\n", __func__, ret);
	return 0;
	}

	if (id_val[0] == 0x03 && id_val[1] != 0) {
	dev_err(&i2c->dev, "smartpa foursemi device\n");
		chip_type = type_foursemi;
	} else {
	dev_err(&i2c->dev, "smartpa tfa device\n");
		chip_type = type_tfa;
	}

	if (chip_type == type_tfa) {
		/* tfa probe */
	dev_err(&i2c->dev, "smartpa invoke tfa98xx_i2c_probe\n");
		ret = tfa98xx_i2c_probe(i2c, id);
	} else{   	/*foursemi probe */
	dev_err(&i2c->dev, "smartpa invoke fs16xx_i2c_probe\n");
		ret = fs16xx_i2c_probe(i2c, id);
	}

	ret = sysfs_create_group(&i2c->dev.kobj, &smartpa_attr_group);
	if (ret)
	printk("%s: Error registering smartpa sysfs", __func__);

	dev_err(&i2c->dev, "smartpa  exit ret = %d\n", ret);
	dev_err(&i2c->dev, "smartpa probe success\n");

	return ret;
}

static int smartpa_i2c_remove(struct i2c_client *client)
{
   int ret = -1;
   if (chip_type == type_tfa)
	 ret = tfa98xx_i2c_remove(client);
   if (chip_type == type_foursemi)
	 ret = fs16xx_i2c_remove(client);

   if (ret)
	dev_err(&client->dev, "%s: smartpa remove failed\n", __func__);

   return 0;
}

static const struct i2c_device_id smartpa_i2c_id[] = {
	{ "smartpa", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, smartpa_i2c_id);

#ifdef CONFIG_OF
static struct of_device_id smartpa_match_tbl[] = {
	{ .compatible = "audio,smartpa" },
	{ },
};
#endif

static struct i2c_driver smartpa_i2c_driver = {
	.driver = {
	.name = "smartpa",
	.owner = THIS_MODULE,
	.of_match_table = of_match_ptr(smartpa_match_tbl),
	},
	.probe =	smartpa_i2c_probe,
	.remove =   smartpa_i2c_remove,
	.id_table = smartpa_i2c_id,
};



static int __init smartpa_modinit(void)
{
	int ret;
	DEBUGPRINT("%s: enter.", __func__);
	ret = i2c_add_driver(&smartpa_i2c_driver);
	if (ret != 0) {
	PRINT_ERROR("Failed to register smartpa I2C driver: %d\n", ret);
	}
	DEBUGPRINT("%s: exit.", __func__);
	return ret;
}
module_init(smartpa_modinit);

static void __exit smartpa_exit(void)
{
	i2c_del_driver(&smartpa_i2c_driver);
}
module_exit(smartpa_exit);

MODULE_DESCRIPTION("ASoC smartpa codec driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Smartpa SW");

