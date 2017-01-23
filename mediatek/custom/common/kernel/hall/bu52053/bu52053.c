/*
 * Copyright (C) 2013 Huawei.
 *
 * Author: yangzicheng <yangzicheng@huawei.com>
 *
 * This program is the driver of hall device.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>
#include <mach/mt_gpio.h>
#include <mach/eint.h>
#include <bu52053.h>
#include <linux/switch.h>
#include <linux/slab.h>

static int bu52053_probe(struct platform_device *pdev);
static int bu52053_remove(struct platform_device *pdev);

#define GPIO_HALL_OUT GPIO_HALL_1_PIN
#define GPIO_HALL_OUT_M_EINT GPIO_HALL_1_PIN_M_EINT

static struct switch_dev hall_det_data = {
    .name = "hall",
	.state = HALL_FAR,

};
static struct work_struct bu52053_eint_work;
struct mutex  hall_mlock;

void bu52053_eint_func(void)
{
	schedule_work(&bu52053_eint_work);
}

static void bu52053_eint_work_callback(struct work_struct *work)
{
	mutex_lock(&hall_mlock);
	if(0 == mt_get_gpio_in(GPIO_HALL_OUT))
	{
		switch_set_state(&hall_det_data,HALL_NEAR);
	    mt_eint_registration(CUST_EINT_MHALL_NUM, HIGH_LEVEL_TRIG, bu52053_eint_func, 0);	
	    //printk("enter workqueue_callback~~~hall_state is HALL_NEAR,change to HIGH_LEVEL_TRIG\n");	
	}
	else if(1 == mt_get_gpio_in(GPIO_HALL_OUT)) 
	{
		switch_set_state(&hall_det_data,HALL_FAR);		
	    mt_eint_registration(CUST_EINT_MHALL_NUM, LOW_LEVEL_TRIG, bu52053_eint_func, 0);
		//printk("enter workqueue_callback~~~hall_state is HALL_FAR,change to LOW_LEVEL_TRIG\n");
	}
	mutex_unlock(&hall_mlock);
}

static int bu52053_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk("bu52053_probe\n");

	ret = switch_dev_register(&hall_det_data);
	if(ret)
	{
		printk("switch_dev_register returned:%d!\n", ret);
		return 1;
	}

    mutex_init(&hall_mlock);
	INIT_WORK(&bu52053_eint_work , bu52053_eint_work_callback);
	
	mt_set_gpio_dir(GPIO_HALL_OUT, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_HALL_OUT, GPIO_HALL_OUT_M_EINT);
	mt_set_gpio_pull_enable(GPIO_HALL_OUT, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_HALL_OUT,GPIO_PULL_UP);
	mt_eint_set_hw_debounce(CUST_EINT_MHALL_NUM, CUST_EINT_MHALL_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_MHALL_NUM, LOW_LEVEL_TRIG, bu52053_eint_func, 0);
	
	mt_eint_unmask(CUST_EINT_MHALL_NUM);
	
	return 0;
}


static int bu52053_remove(struct platform_device *pdev)
{
	
	mt_eint_mask(CUST_EINT_MHALL_NUM);
	cancel_work_sync(&bu52053_eint_work);
	switch_dev_unregister(&hall_det_data);
	return 0;
}

static struct platform_driver bu52053_hall_driver = {
	.probe      = bu52053_probe,
	.remove     = bu52053_remove,    
	.driver     = {
		.name  = "hallsensor",
	}
};



static int __init bu52053_dev_init(void)
{
    printk("Loading bu52053driver\n");
	if(platform_driver_register(&bu52053_hall_driver))
	{
		printk("failed to register bu52053driver");
		return -ENODEV;
	}
	return 0;
}

static void __exit bu52053_dev_exit(void)
{
    printk("Unloading bu52053driver\n");
	platform_driver_unregister(&bu52053_hall_driver);
}

module_init(bu52053_dev_init);
module_exit(bu52053_dev_exit);

MODULE_AUTHOR("HuaWei yangzicheng");
MODULE_DESCRIPTION("HALL device driver");
MODULE_LICENSE("GPL");


