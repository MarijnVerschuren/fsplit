#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>

dev_t					fsplit_dev = 0;
static struct class*	fsplit_class;
static struct cdev		fsplit_cdev;

static int      __init fsplit_init(void);
static void     __exit fsplit_exit(void);
static int      fsplit_open(struct inode *inode, struct file *file);
static int      fsplit_release(struct inode *inode, struct file *file);
static ssize_t  fsplit_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  fsplit_write(struct file *filp, const char *buf, size_t len, loff_t * off);

static struct file_operations fops = {
	.owner      = THIS_MODULE,
	.read       = fsplit_read,
	.write      = fsplit_write,
	.open       = fsplit_open,
	.release    = fsplit_release,
};

static int fsplit_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "fsplit: open()\n");
	return 0;
}

static int fsplit_release(struct inode *inode, struct file *file) {
	printk(KERN_INFO "fsplit: close()\n");
	return 0;
}

static ssize_t fsplit_read(struct file *filp, char __user *buf, size_t len,loff_t * off) {
	printk(KERN_INFO "fsplit: read()\n");
	return 0;
}

static ssize_t fsplit_write(struct file *filp, const char *buf, size_t len, loff_t * off) {
	printk(KERN_INFO "fsplit: write()\n");
	return len;
}

static int __init fsplit_init(void) {
	/*Allocating Major number*/
	if((alloc_chrdev_region(&fsplit_dev, 0, 1, "fsplit")) <0){
		pr_err("fsplit: Cannot allocate major number\n");
		return -1;
	}
	pr_info("fsplit: Major = %d Minor = %d \n", MAJOR(fsplit_dev), MINOR(fsplit_dev));
	/*Creating cdev structure*/
	cdev_init(&fsplit_cdev, &fops);
	/*Adding character device to the system*/
	if((cdev_add(&fsplit_cdev, fsplit_dev, 1)) < 0){
		pr_err("fsplit: Cannot add the device to the system\n");
		goto r_class;
	}
	/*Creating struct class*/
	if(IS_ERR(fsplit_class = class_create("fsplit_class"))){
		pr_err("fsplit: Cannot create the struct class\n");
		goto r_class;
	}
	/*Creating device*/
	if(IS_ERR(device_create(fsplit_class, NULL, fsplit_dev, NULL, "fsplit_device"))){
		pr_err("fsplit: Cannot create the Device 1\n");
		goto r_device;
	}
	pr_info("fsplit: loaded\n");
	return 0;

	r_device:
	class_destroy(fsplit_class);
	r_class:
	unregister_chrdev_region(fsplit_dev, 1);
	return -1;
}

static void __exit fsplit_exit(void) {
	device_destroy(fsplit_class, fsplit_dev);
	class_destroy(fsplit_class);
	cdev_del(&fsplit_cdev);
	unregister_chrdev_region(fsplit_dev, 1);
	pr_info("fsplit: unloaded\n");
}
 
module_init(fsplit_init);
module_exit(fsplit_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marijn Verschuren <marijnverschuren3@gmail.com>");
MODULE_DESCRIPTION("simple linux driver that serves as a simple write through to any other file and log this communication");
MODULE_VERSION("1.0");