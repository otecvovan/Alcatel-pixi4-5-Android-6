/*
 * Copyright (C) 2010 Trusted Logic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <mt-plat/mt_gpio.h>
//#include <mach/eint.h>
//#include <cust_gpio_usage.h>
//#include <cust_eint.h>
//#include <linux/hwmsen_dev.h>
#include <mt_clkbuf_ctl.h>  /*add for clock control*/

#include <linux/device.h>
#include <linux/wakelock.h> /* add by binpeng.huang on 2016.05.29 for Defect 2201390 */


//Why | 0x80000000 here ? because if do not do this, mt_gpio_pin_decrypt will dump_stack~
#define VEN_PIN             (0 | 0x80000000)
#define GPIO4_PIN       	(8 | 0x80000000)
#define IRQ_PIN         	(1 | 0x80000000)
#define CLK_REQ_PIN         (4 | 0x80000000) /* add by binpeng.huang on 2016.05.29 for Defect 2201390 */

#define EINT_NUM                1
#define PN544_I2C_GROUP_ID	2
//#define PN544_WRITE_ID		0x50//0x28
#define PN544_DRVNAME		"pn544"

#define MAX_BUFFER_SIZE		512

#define PN544_MAGIC		0xE9
#define PN544_SET_PWR		_IOW(PN544_MAGIC, 0x01, unsigned int)


struct pn544_dev	{
	wait_queue_head_t	read_wq;
	struct mutex		read_mutex;
	struct i2c_client	*client;
	struct miscdevice	pn544_device;
	bool			irq_enabled;
	bool            clk_irq_enabled; /* add by binpeng.huang on 2016.05.29 for Defect 2201390 */
	spinlock_t		irq_enabled_lock;
	spinlock_t		clk_irq_enabled_lock; /* add by binpeng.huang on 2016.05.29 for Defect 2201390 */
};

static unsigned int pn547_irq;

/* begin: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
static unsigned int pn547_clk_irq;
struct wake_lock    nfc_clk_lock;
/* end: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */

static struct pn544_dev *p_pn544_dev = NULL;
//For DMA
static char *I2CDMAWriteBuf = NULL;
static unsigned int I2CDMAWriteBuf_pa;// = NULL;
static char *I2CDMAReadBuf = NULL;
static unsigned int I2CDMAReadBuf_pa;// = NULL;
static int nfc_exist=0;
extern struct device* get_deviceinfo_dev(void);

struct platform_device nfc_pn547 = {
	.name	       = PN544_DRVNAME,
	.id            = -1,
};
/*----------------------------------------------------------------------------*/

#if 0 // just compatibility
static ssize_t nfc_show(struct device *dev, 
                                  struct device_attribute *attr, char *buf)
{	
    printk("nfc test: nfc show function!\n");

    if (!dev ) {
       
        return 0;
    }
    printk("The nfc is pn547 \n");
    return snprintf(buf, PAGE_SIZE, "%d\n", nfc_exist);    
   //Lightsensor flag,Let the APP know the Psensor is CTP type or physical type// add by llf
}
/*----------------------------------------------------------------------------*/
static ssize_t nfc_store(struct device* dev, 
                                   struct device_attribute *attr, const char *buf, size_t count)
{

	return count;
}   
/*----------------------------------------------------------------------------*/
DEVICE_ATTR(nfc,     S_IWUSR | S_IRUGO, nfc_show, nfc_store);
/*----------------------------------------------------------------------------*/
#endif

static int pn544_compatible_test(struct pn544_dev *pn544_dev)

{
	int ret;
//	struct device * nfc_device;
	unsigned char send_reset[] = {0x20, 0x00, 0x01, 0x01};//PN547 RSET Frame

	  //nfc_device=hwmsen_get_compatible_dev();
	  ///if (device_create_file(nfc_device, &dev_attr_nfc) < 0)
		//printk("Failed to create device file(%s)!\n", dev_attr_nfc.attr.name);

	 // power on
	 mt_set_gpio_out(GPIO4_PIN, 0);
	 mt_set_gpio_out(VEN_PIN, 0);
	 msleep(10);

	 // power off
	 mt_set_gpio_out(GPIO4_PIN, 0);
	 mt_set_gpio_out(VEN_PIN, 1);
	 msleep(10);

         // power on
	 mt_set_gpio_out(GPIO4_PIN, 0);
	 mt_set_gpio_out(VEN_PIN, 0);
	 msleep(10);
 
	// write data, note use dma buffer to transfer, or change the i2c dma flag, if not, will report error
	 memcpy(I2CDMAWriteBuf, send_reset, sizeof(send_reset));

	 ret = i2c_master_send(pn544_dev->client,  (unsigned char *)(uintptr_t)I2CDMAWriteBuf_pa, sizeof(send_reset));

	 // power off
	 mt_set_gpio_out(GPIO4_PIN, 0);
	 mt_set_gpio_out(VEN_PIN, 1);
	 msleep(10);

     if (ret != sizeof(send_reset)) 
	 {
		 printk("pn544 %s : no pn544 or it is bad %d\n", __func__, ret);
		 nfc_exist=0;
		 return -1;
	 }
	 else
	 {
		 printk("pn544 %s :pn544 test OK %d\n", __func__, ret);
		 nfc_exist=1;
		 return 0;
	 }
}


static ssize_t show_pn547_info(struct device *dev, 
                                  struct device_attribute *attr, char *buf)
{
	int len = 0;
	printk("%s\n", __func__);
	if(nfc_exist){
		len = snprintf(buf, PAGE_SIZE, "%s:%s:%s:%s\n", "pn547", "NXP", "sup1", "1");
	}else{
		len = snprintf(buf, PAGE_SIZE, "%s:%s:%s:%s\n", "NA", "NA", "NA", "NA");
	}
	return len;
}
DEVICE_ATTR(nfc, S_IRUGO, show_pn547_info, NULL);


static void pn544_disable_irq(struct pn544_dev *pn544_dev)
{
	unsigned long flags;


	spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	if (pn544_dev->irq_enabled) {
       		disable_irq_nosync(pn547_irq);
		pn544_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);


}

/* begin: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
static void pn544_disable_clk_irq(struct pn544_dev *pn544_dev)
{
	unsigned long flags;

	spin_lock_irqsave(&pn544_dev->clk_irq_enabled_lock, flags);
	if (pn544_dev->clk_irq_enabled) 
	{
		disable_irq_nosync(pn547_clk_irq);
		pn544_dev->clk_irq_enabled = false;
	}
	spin_unlock_irqrestore(&pn544_dev->clk_irq_enabled_lock, flags);
}
/* end: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */

static irqreturn_t  pn544_dev_irq_handler(int irq, void *dev_id)
{
	struct pn544_dev *pn544_dev = p_pn544_dev;

	if (!mt_get_gpio_in(IRQ_PIN)) {

		printk(KERN_DEBUG "***************\n");
		
		return IRQ_HANDLED;
	}


	pn544_disable_irq(pn544_dev);

	/* Wake up waiting readers */
	wake_up(&pn544_dev->read_wq);

        return IRQ_HANDLED;
}

/* begin: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
static irqreturn_t  pn544_dev_clk_irq_handler(int irq, void *dev_id)
{
	struct pn544_dev *pn544_dev = p_pn544_dev;

	printk("%s irq = %d\n", __func__, irq);
	pn544_disable_clk_irq(pn544_dev);

	/* if nfc lock is active, unlock it */
	if (wake_lock_active(&nfc_clk_lock))
	{
		printk("nfc wake_unlock\n");
		wake_unlock(&nfc_clk_lock);
	}
	return IRQ_HANDLED;
}
/* end: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */

static ssize_t pn544_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
	struct pn544_dev *pn544_dev = filp->private_data;
	int ret,i;
//	char tmp[MAX_BUFFER_SIZE];

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	printk("pn544 %s : reading %zu bytes.\n", __func__, count);

	mutex_lock(&pn544_dev->read_mutex);

	if (!mt_get_gpio_in(IRQ_PIN)) {
        
		printk("pn544 read no event\n");
		
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto fail;
		}
		
		printk("pn544 read wait event\n");
		
		pn544_dev->irq_enabled = true;

		enable_irq(pn547_irq);
	
		ret = wait_event_interruptible(pn544_dev->read_wq, mt_get_gpio_in(IRQ_PIN));

		pn544_disable_irq(pn544_dev);

		if (ret) {
			printk("pn544 read wait event error\n");
			goto fail;
		}
	}

	/* begin: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
	/* if NFC need clk  */
	if (mt_get_gpio_in(CLK_REQ_PIN))
	{
		printk("clk_req_pin high\n");

		/* if nfc lock is inactive, wake lock  */
		if (wake_lock_active(&nfc_clk_lock) == 0)
		{
			/* lock system to prevent from deep sleep */
			printk("nfc wake_lock\n");
			//wake_lock(&nfc_clk_lock);
			wake_lock_timeout(&nfc_clk_lock, 3600 * HZ); /* it will auto unlock in 60min */

			if (pn544_dev->clk_irq_enabled == false)
			{
				printk("enable nfc clk req irq\n");
				pn544_dev->clk_irq_enabled = true;
				enable_irq(pn547_clk_irq);
			}
		}
	}
	/* end: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */

	/* Read data */
	
    ret = i2c_master_recv(pn544_dev->client,  (unsigned char *)(uintptr_t)I2CDMAReadBuf_pa, count);
	   
	mutex_unlock(&pn544_dev->read_mutex);


	if (ret < 0) {
		pr_err("pn544 %s: i2c_master_recv returned %d\n", __func__, ret);
		return ret;
	}
	if (ret > count) {
		pr_err("pn544 %s: received too many bytes from i2c (%d)\n", __func__, ret);
		return -EIO;
	}
	
   if (copy_to_user(buf, I2CDMAReadBuf, ret)) 
   {
      printk(KERN_DEBUG "%s : failed to copy to user space\n", __func__);
      return -EFAULT;
   }

	printk("pn544 IFD->PC:");
	for(i = 0; i < ret; i++) {
		printk(" %02X", I2CDMAReadBuf[i]);
	}
	printk("\n");

	return ret;

fail:
	mutex_unlock(&pn544_dev->read_mutex);
	return ret;
}

static ssize_t pn544_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
	struct pn544_dev *pn544_dev;
	int ret, i,idx = 0;

	pn544_dev = p_pn544_dev;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(I2CDMAWriteBuf, &buf[(idx*255)], count)) 
	{
		printk(KERN_DEBUG "%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}
	printk("pn544 %s : writing %zu bytes.\n", __func__, count);
	/* Write data */
      ret = i2c_master_send(pn544_dev->client,  (unsigned char *)(uintptr_t)I2CDMAWriteBuf_pa, count);
	if (ret != count) {
		pr_err("pn544 %s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}
	printk("pn544 PC->IFD:");
	for(i = 0; i < count; i++) {
		printk(" %02X\n", I2CDMAWriteBuf[i]);
	}
	printk("\n");
	

	return ret;
}

static int pn544_dev_open(struct inode *inode, struct file *filp)
{
	struct pn544_dev *pn544_dev = container_of(filp->private_data,
						struct pn544_dev,
						pn544_device);

	filp->private_data = pn544_dev;
	
	pr_debug("pn544 %s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	return 0;
}

static long pn544_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
		case PN544_SET_PWR:
			if (arg == 2) {
				/* power on with firmware download (requires hw reset)
				 */

                // enable nfc clock buffer
                clk_buf_ctrl(CLK_BUF_NFC, 1);
				
				printk("pn544 %s power on with firmware\n", __func__);
				mt_set_gpio_out(VEN_PIN, 0);
			printk(" ppn544_dev_ioctl_VEN_PIN=%d\n",mt_get_gpio_out(VEN_PIN));
				mt_set_gpio_out(GPIO4_PIN, 1);
			printk(" ppn544_dev_ioctl_GPIO4_PIN=%d\n",mt_get_gpio_out(GPIO4_PIN));
				msleep(10);
				mt_set_gpio_out(VEN_PIN, 1);
			printk(" ppn544_dev_ioctl_VEN_PIN=%d\n",mt_get_gpio_out(VEN_PIN));
				msleep(50);
				mt_set_gpio_out(VEN_PIN, 0);
			printk(" ppn544_dev_ioctl_VEN_PIN=%d\n",mt_get_gpio_out(VEN_PIN));
				msleep(10);
			} else if (arg == 1) {

                // enable nfc clock buffer
                clk_buf_ctrl(CLK_BUF_NFC, 1);
			
				/* power on */
				printk("pn544 %s power on\n", __func__);
				mt_set_gpio_out(GPIO4_PIN, 0);
				mt_set_gpio_out(VEN_PIN, 0);
				msleep(10);
			} else  if (arg == 0) {
				/* power off */
                // disable nfc clock buffer
                clk_buf_ctrl(CLK_BUF_NFC, 0);
				
				printk("pn544 %s power off\n", __func__);
				mt_set_gpio_out(GPIO4_PIN, 0);
				mt_set_gpio_out(VEN_PIN, 1);
				msleep(10);
			} else {
				printk("pn544 %s bad arg %lu\n", __func__, arg);
				return -EINVAL;
			}
			break;
		default:
			printk("pn544 %s bad ioctl %u\n", __func__, cmd);
			return -EINVAL;
	}

	return 0;
}

static const struct file_operations pn544_dev_fops = {
	.owner	= THIS_MODULE,
	.llseek	= no_llseek,
	.read	= pn544_dev_read,
	.write	= pn544_dev_write,
	.open	= pn544_dev_open,
	.unlocked_ioctl  = pn544_dev_ioctl,
};


extern unsigned int irq_of_parse_and_map(struct device_node *dev, int index);

static int pn544_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct pn544_dev *pn544_dev;
	struct device_node *node;
	struct device *deviceinfo=NULL;

	printk("pn544 nfc probe step01 is ok\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("pn544 %s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

	printk("pn544 nfc probe step02 is ok\n");

	pn544_dev = kzalloc(sizeof(*pn544_dev), GFP_KERNEL);
	if (pn544_dev == NULL) {
		dev_err(&client->dev, "pn544 failed to allocate memory for module data\n");
		return -ENOMEM;
	}
	memset(pn544_dev, 0, sizeof(struct pn544_dev));
	p_pn544_dev = pn544_dev;

	printk("pn544 nfc probe step03 is ok\n");
	
	client->addr = (client->addr & I2C_MASK_FLAG);
	client->addr = (client->addr | I2C_DMA_FLAG);
    client->addr = (client->addr | I2C_DIRECTION_FLAG);
	client->timing = 400;
	pn544_dev->client = client;

	/* init mutex and queues */
	init_waitqueue_head(&pn544_dev->read_wq);
	mutex_init(&pn544_dev->read_mutex);
	spin_lock_init(&pn544_dev->irq_enabled_lock);
	/* begin: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
	spin_lock_init(&pn544_dev->clk_irq_enabled_lock);
	wake_lock_init(&nfc_clk_lock, WAKE_LOCK_SUSPEND, "nfc clock wakelock");
	/* end: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */

	pn544_dev->pn544_device.minor = MISC_DYNAMIC_MINOR;
	pn544_dev->pn544_device.name = PN544_DRVNAME;
	pn544_dev->pn544_device.fops = &pn544_dev_fops;

	ret = misc_register(&pn544_dev->pn544_device);
	if (ret) {
		pr_err("pn544 %s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}
    
	printk("pn544 nfc probe step04 is ok\n");
#ifdef CONFIG_64BIT
	I2CDMAWriteBuf = (char *)dma_alloc_coherent(&client->dev, MAX_BUFFER_SIZE, (dma_addr_t *)&I2CDMAWriteBuf_pa, GFP_KERNEL);
#else
	I2CDMAWriteBuf = (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE, &I2CDMAWriteBuf_pa, GFP_KERNEL);
#endif
	if (I2CDMAWriteBuf == NULL) 
	{
		pr_err("%s : failed to allocate dma buffer\n", __func__);
		goto err_request_irq_failed;
	}
	
#ifdef CONFIG_64BIT // refer to MT6605
	I2CDMAReadBuf = (char *)dma_alloc_coherent(&client->dev, MAX_BUFFER_SIZE, (dma_addr_t *)&I2CDMAReadBuf_pa, GFP_KERNEL);
#else
	I2CDMAReadBuf = (char *)dma_alloc_coherent(NULL, MAX_BUFFER_SIZE, &I2CDMAReadBuf_pa, GFP_KERNEL);
#endif
	if (I2CDMAReadBuf == NULL) 
	{
		pr_err("%s : failed to allocate dma buffer\n", __func__);
		goto err_request_irq_failed;
	}
        printk(KERN_DEBUG "%s :I2CDMAWriteBuf_pa %x, I2CDMAReadBuf_pa,%x\n", __func__, I2CDMAWriteBuf_pa, I2CDMAReadBuf_pa);

	/* VEN */
	mt_set_gpio_mode(VEN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(VEN_PIN, GPIO_DIR_OUT);
    
	/* GPIO4 */
	mt_set_gpio_mode(GPIO4_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO4_PIN, GPIO_DIR_OUT);
    
       mt_set_gpio_mode(IRQ_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(IRQ_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(IRQ_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(IRQ_PIN, GPIO_PULL_DOWN);
    
	/* begin: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
	/* clk request pin init */
	mt_set_gpio_mode(CLK_REQ_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(CLK_REQ_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(CLK_REQ_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(CLK_REQ_PIN, GPIO_PULL_DOWN);
	/* end: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */

	printk("pn544 nfc probe step05 is ok\n");
    
	pn544_dev->irq_enabled = true;

       node = of_find_compatible_node(NULL, NULL, "mediatek,pn547-nfc");

	if (node) {
		pn547_irq = irq_of_parse_and_map(node, 0);

		client->irq = pn547_irq;

                ret = request_irq(pn547_irq,  pn544_dev_irq_handler, IRQF_TRIGGER_HIGH, "pn547-nfc-eint", NULL);

	if (ret) {

              ret = -1;
              printk("%s : requesting IRQ error\n", __func__);

	} 

	} else {
		printk("%s : can not find NFC eint compatible node\n",  __func__);
	}

	
	pn544_disable_irq(pn544_dev);

	/* begin: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
	pn544_dev->clk_irq_enabled = true;
    node = of_find_compatible_node(NULL, NULL, "mediatek,pn547-irq-nfc");
    
    if (node)
    {
	    printk("pn544 find irq-nfc node\n");
	    pn547_clk_irq = irq_of_parse_and_map(node, 0);
	    ret = request_irq(pn547_clk_irq,  pn544_dev_clk_irq_handler, IRQF_TRIGGER_LOW, "pn547-nfc-clk-eint", NULL);

	    if (ret)
	    {
		    ret = -1;
		    printk("pn544 register clk irq error\n");
	    }
    }
    else
    {
		printk("%s : can not find pn547-irq-nfc node\n",  __func__);
    }
	pn544_disable_clk_irq(pn544_dev);
	/* end: add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */

	printk("%s : requesting IRQ %d\n", __func__, client->irq);
	i2c_set_clientdata(client, pn544_dev);
	
	printk("pn544 nfc probe step06 is ok\n");

	// Check if pn547 chips exist
	pn544_compatible_test(p_pn544_dev);
	
	//add for device information
	deviceinfo = get_deviceinfo_dev();
	if(deviceinfo){
		if (device_create_file(deviceinfo, &dev_attr_nfc) < 0)
			pr_err("Failed to create device file(%s)!\n", dev_attr_nfc.attr.name);
	}
	else{
		pr_err("[pn547] Failed to get deviceinfo device!\n");
	}
	
	printk("pn544 nfc probe step07 is ok\n");

        //if nfc_exist ==0 ,remove the dev/pn544 
	if (0==nfc_exist) {
		pr_err("pn544 %s : the pn544 is not exit, misc_register failed\n", __FILE__);
		misc_deregister(&pn544_dev->pn544_device);
	}
	return 0;

//err_dma_alloc:
//	misc_deregister(&pn544_dev->pn544_device);
err_misc_register:
	mutex_destroy(&pn544_dev->read_mutex);
	kfree(pn544_dev);
	p_pn544_dev = NULL;

err_request_irq_failed:
	misc_deregister(&pn544_dev->pn544_device);   
	return ret;
}

static int pn544_remove(struct i2c_client *client)
{
	struct pn544_dev *pn544_dev;

	if (I2CDMAWriteBuf)
	{
#ifdef CONFIG_64BIT 	
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, I2CDMAWriteBuf, I2CDMAWriteBuf_pa);
#else
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, I2CDMAWriteBuf, I2CDMAWriteBuf_pa);
#endif
		I2CDMAWriteBuf = NULL;
		I2CDMAWriteBuf_pa = 0;
	}

	if (I2CDMAReadBuf)
	{
#ifdef CONFIG_64BIT
		dma_free_coherent(&client->dev, MAX_BUFFER_SIZE, I2CDMAReadBuf, I2CDMAReadBuf_pa);
#else
		dma_free_coherent(NULL, MAX_BUFFER_SIZE, I2CDMAReadBuf, I2CDMAReadBuf_pa);
#endif
		I2CDMAReadBuf = NULL;
		I2CDMAReadBuf_pa = 0;
	}

    wake_lock_destroy(&nfc_clk_lock); /* add by binpeng.huang.hz@tcl.com on 2016.05.29 for Defect 2201390 */
	pn544_dev = i2c_get_clientdata(client);
	misc_deregister(&pn544_dev->pn544_device);
	mutex_destroy(&pn544_dev->read_mutex);
	kfree(pn544_dev);
	p_pn544_dev = NULL;
	
	return 0;
}

static const struct i2c_device_id pn544_id[] = { { PN544_DRVNAME, 0 }, {} };
MODULE_DEVICE_TABLE(i2c, pn544_id);

static const struct of_device_id nfc_switch_of_match[] = {{.compatible = "mediatek,NFC"}, {}, };

static int pn544_detect(struct i2c_client *client, struct i2c_board_info *info) 
{         
	printk(" pn544_detect \n");
	strcpy(info->type, PN544_DRVNAME);
	return 0;
}

static struct i2c_driver pn544_driver = {
	.id_table	= pn544_id,
	.probe		= pn544_probe,
	.remove		= pn544_remove,
	.detect     = pn544_detect,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= PN544_DRVNAME,
		.of_match_table = nfc_switch_of_match,
	},
};

/*
 * module load/unload record keeping
 */
 static int pn544_platform_probe(struct platform_device *pdev) 
{
         int ret ;
	 printk("add pn544_driver\n");
	 
	 ret = i2c_add_driver(&pn544_driver);
	 if(ret !=0)
        {
            printk("add pn544_driver fail\n");
            return -1;
        }	
	 return 0;
}
 static int pn544_platform_remove(struct platform_device *pdev) 
{
	 return 0;
}

static struct platform_driver pn544_platform_driver = {
	.probe      = pn544_platform_probe,
	.remove     = pn544_platform_remove,    
	.driver     = 
	{
		.name  = PN544_DRVNAME,
		.owner = THIS_MODULE,
	}
};
static int __init pn544_dev_init(void)
{
        int ret;
		
	printk("pn544_dev_init\n");
	
	ret = platform_device_register(&nfc_pn547);
	if( ret){
		printk("platform_device_register pn544 failed!\n");
		return ret;
	}
	
	if(platform_driver_register(&pn544_platform_driver)){
		printk("platform_driver_register failed\n");
                return -ENODEV;
	}
	return 0;
}
module_init(pn544_dev_init);

static void __exit pn544_dev_exit(void)
{
	printk("pn544 Unloading pn544 driver\n");
	i2c_del_driver(&pn544_driver);
}
module_exit(pn544_dev_exit);

MODULE_AUTHOR("XXX");
MODULE_DESCRIPTION("NFC PN544 driver");
MODULE_LICENSE("GPL");
