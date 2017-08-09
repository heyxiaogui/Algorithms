#include <linux/module.h>
#include <linux/init>  
#include <linux/types.h>  
#include <linux/fs.h>  
#include <linux/errno.h>  
#include <linux/mm.h>  
#include <linux/sched.h>   
#include <linux/cdev.h>  
#include <asm/io.h>  
#include <asm/uaccess.h>  
#include <linux/timer.h>  
#include <asm/atomic.h>  
#include <linux/slab.h>  
#include <linux/device.h>

#ifndef KDEDUG
#define KDEDUG
#endif

#ifdef KDEDUG
#define DRV_PRINT(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DRV_PRINT(fmt, ...)
#endif


#define DEVICE_NAME "myCharDev"
#define DEV_SIZE 10240

module_param(device_major, int, S_IRUGO);

struct char_dev
{
	struct cdev cdev;
	char *data;
	char *p_read;
	char *p_write;
	unsigned int count;
	unsigned int size;
};

struct char_dev *p_chardev;
struct class *dev_class;

static int chardev_open(struct inode *inode, struct file *filp)
{
	struct char_dev *p_dev;
	p_dev = container_of(inode->i_cdev, struct char_dev, cdev);
	filp->private_data = p_dev;
/*tqj_2017.8.9:   Not defined some open mode yet*/	
	return 0;

}



static const struct file_operations chardev_fops{
	.owner   = THIS_MODULE;
	.read    = chardev_read;
	.write   = chardev_write;
	.open    = chardev_open;
	.release = chardev_release;
};



static int alloc_chardev(struct char_dev *chardev, int size)
{	
	chardev->data = kmalloc(size, GFP_KERNEL);
	if (!chardev)
	{
		printk(KERN_EMERG "fail to alloc char device buffer\n" );
		return -1;
	}
	chardev->size = size;
	chardev->count = 0;
	chardev->p_read = chardev->data;
	chardev-p_write = chardev->data;

	return 0;
}



static void chardev_setup_cdev(struct char_dev *p_cdev)
{
	int ret;
	dev_t devno;
	devno = MKDEV(device_major, 0);

	cdev_init(&p_cdev->cdev, chardev_fops);
	p_cdev->cdev.owner = THIS_MODULE;
	p_cdev->cdev.ops   = &chardev_fops;

	ret = cdev_add(&p_cdev->cdev, devno, 1);
	if (ret)
	{
		printk(KERN_EMERG "ERROR: %d adding char_cdev: %d\n", ret, device_major);
		lcdev_del(&p_cdev->cdev);
	}
}



static int __init chardev_init(void)
{
	int ret;
	dev_t devno;
	if (device_major)
	{
		devno = MKDEV(device_major, 0);
		ret = register_chrdev_region(devno, 1, DEVICE_NAME);
	}
	else
	{
		ret = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
		device_major = MAJOR(devno);
	}

	if (ret < 0)
	{
		printk(KERN_EMERG "char_dev: failed to alloc device number by the major %d\n", device_major);
		return ret;
	}

	p_chardev = kmalloc(sizeof(struct char_dev), GFP_KERNEL);
	if (!p_chardev)
	{
		printk(KERN_EMERG "failed to alloc p_chardev\n");
		goto kmalloc_failed;
	}

	memset(p_chardev, 0, sizeof(struct char_dev));

	ret = alloc_chardev(p_chardev, DEV_SIZE);
	if (ret)
	{
		goto kmalloc_failed;
	}


	chardev_setup_cdev(p_chardev);

	dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	device_create(dev_class, NULL, MKDEV(device_major ,0), NULL, DEVICE_NAME); 

	kmalloc_failed:
		unregister_chrdev_region(devno, 1);
}

static void __exit chardev_exit(void)
{
	cdev_del(&p_chardev->cdev);
	kfree(p_chardev);
	device_destory(dev_class, MKDEV(device_major ,0));
	class_destory(dev_class);
	unregister_chrdev_region(MKDEV(device_major, 0), 1);
}

module_init(chardev_init);
module_exit(chardev_exit);
MODULE_LICENSE("GPL");
MOUDLE_AUTHOR("heyxiaogui");