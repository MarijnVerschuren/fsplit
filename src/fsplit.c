//
// Created by marijn on 12/17/24.
//
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include <net/sock.h>

#include "config.h"



/*!<
 * type declarations
 * */
typedef unsigned char uint8_t;



/*!<
 * function declarations
 * */
static int      	fsplit_open(struct inode *inode, struct file *file);
static int      	fsplit_release(struct inode *inode, struct file *file);
static ssize_t  	fsplit_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  	fsplit_write(struct file *filp, const char *buf, size_t len, loff_t * off);

static void			socket_receive(struct sk_buff* skb);

static __init int	fsplit_init(void);
static __exit void	fsplit_exit(void);



/*!<
 * variables
 * */
dev_t					fsplit_dev =		0;
static struct class*	fsplit_class =		NULL;
static struct cdev		fsplit_cdev =		{};

static struct file_operations fops = {
	.owner =	THIS_MODULE,
	.read =		fsplit_read,
	.write =	fsplit_write,
	.open =		fsplit_open,
	.release =	fsplit_release,
};

static struct sock*		socket =			NULL;



/*!<
 * file functions
 * */
static int fsplit_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "fsplit: open() -> %d\n");
	return 0;
}

static int fsplit_release(struct inode *inode, struct file *file) {
	printk(KERN_INFO "fsplit: close()\n");
	return 0;
}

static ssize_t fsplit_read(struct file *filp, char __user *buf, size_t len,loff_t * off) {
	printk(KERN_INFO "fsplit: read\n");
	return 0;
}

static ssize_t fsplit_write(struct file *filp, const char *buf, size_t len, loff_t * off) {
	printk(KERN_INFO "fsplit: write\n");
	return len;
}



/*!<
 * socket functions
 * */
static void socket_receive(struct sk_buff* sock_buf) {
	struct nlmsghdr*		netlink_hdr =		NULL;
	int						pid =				0;
	struct sk_buff*			sock_buf_out =		NULL;
	int						res =				0;

	// TODO
	char*					msg =				"Hello from kernel";
	int						msg_size =			18;

	printk(KERN_INFO "fsplit: socket receive\n");

	netlink_hdr = (struct nlmsghdr*)sock_buf->data;
	pid = netlink_hdr->nlmsg_pid;
	printk(KERN_INFO "fsplit received msg from PID: %d\n", pid);
	printk(KERN_INFO "fsplit msg: %s\n", (char*)nlmsg_data(netlink_hdr));

	sock_buf_out = nlmsg_new(msg_size, 0);

	if(!sock_buf_out) {
		printk(KERN_ERR "fsplit: failed to allocate message buffer\n");
		return;
	}

	netlink_hdr = nlmsg_put(sock_buf_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(sock_buf_out).dst_group = 0; /* not in mcast group */
	strncpy(nlmsg_data(netlink_hdr), msg, msg_size);

	res = nlmsg_unicast(socket, sock_buf_out, pid);

	if(res < 0) {
		printk(KERN_INFO "fsplit: error while transmitting on socket\n");
	}
}



/*!<
 * init functions
 * */
static char* fsplit_devnode(const struct device* dev, umode_t* mode) {
	if (!mode) {
		return NULL;
	}
	*mode = 0666;
	return NULL;
}


static __init int fsplit_init(void) {
	if((alloc_chrdev_region(&fsplit_dev, 0, 1, "fsplit")) < 0) {
		printk(KERN_ERR "fsplit: cannot allocate major number\n");
		return -1;
	}
	printk(KERN_INFO "fsplit: %d.%d\n", MAJOR(fsplit_dev), MINOR(fsplit_dev));

	cdev_init(&fsplit_cdev, &fops);
	if((cdev_add(&fsplit_cdev, fsplit_dev, 1)) < 0){
		printk(KERN_ERR "fsplit: cannot add the device to the system\n");
		goto err_cdev;
	}

	if(IS_ERR(fsplit_class = class_create("fsplit_class"))){
		printk(KERN_ERR "fsplit: cannot create the struct class\n");
		goto err_class;
	}

	fsplit_class->devnode = fsplit_devnode;
	if(IS_ERR(device_create(fsplit_class, NULL, fsplit_dev, NULL, "fsplit"))){
		printk(KERN_ERR "fsplit: cannot create character device\n");
		goto err_device;
	}

	struct netlink_kernel_cfg sock_cfg = {
		.input =	socket_receive
	};
	socket = netlink_kernel_create(&init_net, NETLINK_PORT, &sock_cfg);
	if (!socket) {
		printk(KERN_ERR "fsplit: cannot create socket\n");
		goto err_socket;
	}
	
	printk(KERN_INFO "fsplit: loaded\n");
	return 0;

	err_socket:
	device_destroy(fsplit_class, fsplit_dev);
	err_device:
	class_destroy(fsplit_class);
	err_class:
	cdev_del(&fsplit_cdev);
	err_cdev:
	unregister_chrdev_region(fsplit_dev, 1);
	return -1;
}

static __exit void fsplit_exit(void) {
	netlink_kernel_release(socket);
	device_destroy(fsplit_class, fsplit_dev);
	class_destroy(fsplit_class);
	cdev_del(&fsplit_cdev);
	unregister_chrdev_region(fsplit_dev, 1);
	printk(KERN_INFO "fsplit: unloaded\n");
}



/*!<
 * kernel macros
 * */
module_init(fsplit_init);
module_exit(fsplit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marijn Verschuren <marijnverschuren3@gmail.com>");
MODULE_DESCRIPTION("simple linux driver that serves as a simple write through to any other file and log this communication");
MODULE_VERSION("1.0");