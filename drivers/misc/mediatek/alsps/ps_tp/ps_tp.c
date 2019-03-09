/*
 * Author: yucong xiong <yucong.xion@mediatek.com>
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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>

#include "cust_alsps.h"
//#include "ps_tp.h"
#include "alsps.h"
#ifdef CUSTOM_KERNEL_SENSORHUB
#include <SCP_sensorHub.h>
#endif
#include <linux/ioctl.h>


/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define APS_TAG				  "[ps_tp] "
#define APS_ERR(fmt, args...)	pr_err(APS_TAG fmt, ##args)
#define APS_LOG(fmt, args...)	pr_debug(APS_TAG fmt, ##args)

/*----------------------------------------------------------------------------*/
struct ps_tp_priv {
	struct alsps_hw  *hw;
	struct i2c_client *client;
	struct work_struct	eint_work;
#ifdef CUSTOM_KERNEL_SENSORHUB
	struct work_struct init_done_work;
#endif

	/*misc*/
	u16		als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on;	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t	trace;
	atomic_t  init_done;
	struct device_node *irq_node;
	int		irq;

	/*data*/
	u16			als;
	u8			ps;
	u8			_align;
	u16			als_level_num;
	u16			als_value_num;
	u32			als_level[C_CUST_ALS_LEVEL-1];
	u32			als_value[C_CUST_ALS_LEVEL];
	int			ps_cali;

	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_low;	/*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_high;	 /*the cmd value can't be read, stored in ram*/
	atomic_t	als_thd_val_low;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val;
	ulong		enable;		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/

};
/*----------------------------------------------------------------------------*/

struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
static struct ps_tp_priv *ps_tp_obj;

static int ps_tp_local_init(void);
static int ps_tp_remove(void);
static int ps_tp_init_flag =  -1;
static struct alsps_init_info ps_tp_init_info = {
		.name = "ps_tp",
		.init = ps_tp_local_init,
		.uninit = ps_tp_remove,

};

extern int msg22xx_pls_enable(int);
                
extern int  get_msg22xx_data(void );
                
extern int fts_enable_ps(int enable);
 
extern int fts_get_ps_value(void);


int tp_vendor_id_ps;

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static int pls_enable(void)
{
        printk("%s\n", __func__);
        if(tp_vendor_id_ps==0) //tp is msg22xx;
                return msg22xx_pls_enable(1);
	if(tp_vendor_id_ps==1) //tp si ft6336
		return fts_enable_ps(1);

	return -1;
}

static int pls_disable(void)
{
        printk("%s\n", __func__);
        if(tp_vendor_id_ps==0) //tp is msg22xx;
                return msg22xx_pls_enable(0);
        if(tp_vendor_id_ps==1) //tp is ft6336;
		return fts_enable_ps(0);

	return -1;
}
static int ps_tp_open(struct inode *inode, struct file *file)
{
	return 0;
}
/************************************************************/
static int ps_tp_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/************************************************************/
static const struct file_operations ps_tp_fops = {
	.owner = THIS_MODULE,
	.open = ps_tp_open,
	.release = ps_tp_release,
};

static struct miscdevice ps_tp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ps_tp_fops,
};

static int ps_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */

static int ps_enable_nodata(int en)
{
        int err=0;
        printk("%s\n", __func__);
        if(en)
        {
                if(err != pls_enable())
                {
                        printk("enable ps fail: %d\n", err);
                        return -1;
                }
        }
        else
        {
                if(err != pls_disable())
                {
                        printk("disable ps fail: %d\n", err);
                        return -1;
                }
        }

        return 0;
}

static int ps_set_delay(u64 ns)
{
	return 0;
}

static int ps_get_data(int *value, int *status)
{
	int err = 0;
        if(tp_vendor_id_ps==0) //tp is msg22xx;
                *value= get_msg22xx_data();
        if(tp_vendor_id_ps==1) //tp is ft6336;
                *value= fts_get_ps_value();
	
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	printk("[%s]:*value = %d\n", __func__, *value);
	return err;
}


/*-----------------------------------i2c operations----------------------------------*/
static int ps_tp_local_init(void)
{
	struct ps_tp_priv *obj;
	//struct als_control_path als_ctl = {0};
	//struct als_data_path als_data = {0};
	struct ps_control_path ps_ctl = {0};
	struct ps_data_path ps_data = {0};
	int err = 0;

	APS_LOG("ps_tp_i2c_probe\n");

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(*obj));
	ps_tp_obj = obj;

	err = misc_register(&ps_tp_device);
	if (err) {
		APS_ERR("ps_tp_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	ps_ctl.open_report_data = ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay  = ps_set_delay;
	ps_ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	ps_ctl.is_support_batch = obj->hw->is_batch_supported_ps;
#else
	ps_ctl.is_support_batch = false;
#endif
	err = ps_register_control_path(&ps_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	err = batch_register_support_info(ID_PROXIMITY, ps_ctl.is_support_batch, 1, 0);
	if (err)
		APS_ERR("register proximity batch support err = %d\n", err);

	ps_tp_init_flag = 0;
	APS_LOG("%s: OK\n", __func__);
	return 0;

exit_sensor_obj_attach_fail:
exit_misc_device_register_failed:
		misc_deregister(&ps_tp_device);
		kfree(obj);
exit:
	APS_ERR("%s: err = %d\n", __func__, err);
	ps_tp_init_flag =  -1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int ps_tp_remove(void)
{
	return 0;
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static int __init ps_tp_init(void)
{
	const char *name = "mediatek,ps_tp";

	hw =   get_alsps_dts_func(name, hw);
	if (!hw)
		APS_ERR("get dts info fail\n");
	alsps_driver_add(&ps_tp_init_info);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ps_tp_exit(void)
{
}
/*----------------------------------------------------------------------------*/
module_init(ps_tp_init);
module_exit(ps_tp_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("hugo.deng");
MODULE_DESCRIPTION("ps_tp driver");
MODULE_LICENSE("GPL");

