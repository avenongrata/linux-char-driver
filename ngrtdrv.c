#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/kdev.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define DRIVER_NAME "chrdriver"

static int chr_major = 0;
module_param(chr_major, int, 0644);
MODULE_PARM_DESC(chr_major, "Allows you to set own chr_major number");

static int chr_minor = 0;
module_param(chr_minor, int, 0644); 
MODULE_PARM_DESC(chr_major, "Allows you to set own chr_major number");

struct chr_session {
	char *buf;
	unsigned long byte_read;
	unsigned long byte_write;
	int cur_session;
	unsigned int buf_len;
};

struct chr_dev {
	dev_t devt;
	struct cdev cdev;
	int session_count;
	struct mutex chr_mutex;
	struct chr_session *session;
	unsigned long total_byte_read;
	unsigned long total_byte_write;
} chrdev;

static int chr_open(struct inode *inode, struct file *filp)
{
	struct chr_dev *dev;

	pr_debug("%s: Driver opened\n", DRIVER_NAME);

	dev = container_of(inode->i_cdev, struct chr_dev, cdev);
	filp->private_data = dev;

	dev->session = kmalloc(sizeof (struct chr_session), GFP_KERNEL);
	if (!dev->session) {
		pr_debug("%s: Can't allocate memmory for session\n", DRIVER_NAME);
		return -1;
	}
	dev->session->byte_read = 0;
	dev->session->byte_write = 0;
	dev->session->buf_len = 1000;

	mutex_lock(&dev->chr_mutex);
	dev->session_count++;
	dev->session->cur_session = dev->session_count;
	mutex_unlock(&dev->chr_mutex);

	return 0;
}

static int chr_release(struct inode *inode, struct file *filp)
{	
	struct chr_dev *dev;

	pr_debug("%s: Driver closed\n", DRIVER_NAME);

	dev = filp->private_data;
	mutex_lock(&dev->chr_mutex);
	dev->total_byte_read += dev->session->byte_read;
	dev->total_byte_write += dev->session->byte_write;
	mutex_unlock(&dev->chr_mutex);

	if (dev->session->buf)
		kfree(dev->session->buf);
	kfree(dev->session);

	return 0;
}

static ssize_t chr_read(struct file *filp, char __user *buf, size_t count, loff_f *pos)
{
	struct chr_dev *dev;

	pr_debug("%s: Read function\n", DRIVER_NAME);

	dev = filp->private_data;
	if (!dev->session->byte_write)
		return 0;		// nothing was written

	if ()

}

static ssize_t chr_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{

	pr_debug("%s: Write function\n", DRIVER_NAME);

		dev->session->buf = kmalloc(1000, GFP_KERNEL);
	if (!dev->session->buf) {
		pr_debug("%s: Can't allocate memmory for buffer\n", DRIVER_NAME);
		kfree(dev->session);
		return -1;
	}


}

static long chr_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

}

static const struct file_operations fops = {
	.owner			= THIS_MODULE,
	.open 			= chr_open,
	.release 		= chr_release,
	.read			= chr_read,
	.write			= chr_write,
	.unlocked_ioctl	= chr_unlocked_ioctl,
};

static int __init chr_init(void)
{
	int ret;

	if (chr_major) {
		chrdev.devt = MKDEV(chr_major, chr_minor);
		ret = register_chrdev_region(chrdev.devt, 1, DRIVER_NAME);
	} else {
		ret = alloc_chrdev_region(&chrdev.devt, chr_minor, 1, DRIVER_NAME);
		chr_major = MAJOR(chrdev.devt);
	}

	if (ret < 0) {
		pr_debug("%s: Can't register char device region\n", DRIVER_NAME);
		return ret;
	} else
		pr_debug("%s: Char device has own region\n", DRIVER_NAME);

	cdev_init(&chrdev.cdev, &fops);
	ret = cdev_add(&chrdev.cdev, chrdev.devt, 1);
	if (ret < 0) {
		pr_debug("%s: Can't add char device to system\n", DRIVER_NAME);
		return ret;
	} else 
		pr_debug("%s: Char device has been registered in system\n", DRIVER_NAME);

	chrdev.session_count = 0;
	chrdev.session = NULL;
	chrdev.total_byte_read = 0;
	chrdev.total_byte_write = 0;
	mutex_init(&chrdev.chr_mutex);

	return 0;
}

static void __exit chr_exit(void)
{
	cdev_del(&chrdev.cdev);
	unregister_chrdev_region(chrdev.devt, 1);

}

module_init(chr_init);
module_exit(chr_exit);

MODULE_LICENSE("GPL");