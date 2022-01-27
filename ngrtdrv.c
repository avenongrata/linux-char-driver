/*
 * Linux char driver
 *
 * Copyright (C) 2021 Kirill Yustitskii
 *
 * Authors:  Kirill Yustitskii  <inst: yustitskii_kirill>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>

/*----------------------------------------------------------------------------*/

#define DRIVER_NAME "ngrtdrv"

/*----------------------------------------------------------------------------*/

static int chr_major = 0;
module_param(chr_major, int, 0644);
MODULE_PARM_DESC(chr_major, "Allows you to set own chr_major number");

static int chr_minor = 0;
module_param(chr_minor, int, 0644); 
MODULE_PARM_DESC(chr_major, "Allows you to set own chr_major number");

/*----------------------------------------------------------------------------*/

struct chr_session
{
    unsigned int session_id;        /* session id */
    char *buf;                      /* buffer for data */
    size_t buf_len;                 /* max buffer len */
    size_t byte_read;               /* how many bytes were read */
    size_t byte_write;              /* how many bytes were written */
};

struct chr_dev
{
    struct mutex chr_mutex;         /* avoid race condition for session id */ 
    struct chr_session *session;    /* current session */
    int session_count;              /* count of sessions */
    size_t total_byte_read;         /* total bytes read*/
    size_t total_byte_write;        /* total written bytes */
    
    struct cdev cdev;               /* char device */    
    struct class *cl;               /* char device class  */
    struct device *device;          /* device associated with char_device */
    dev_t devt;                     /* char device number */
} chrdev;

/*----------------------------------------------------------------------------*/

static size_t get_max_byte(struct chr_dev *dev, size_t count)
{   
    return dev->session->buf_len - dev->session->byte_write - 1;
}

/*----------------------------------------------------------------------------*/

/* for cleanup handling */
enum chr_clean
{
    CHR_CHRDEV_DELETE,
    CHR_DEVICE_DESTROY,
    CHR_CLASS_DESTROY,
    CHR_UNREGISTER_CHRDEV
};

void cleanup_handler(struct chr_dev *dev, enum chr_clean index)
{
    switch (index)
    {
    case CHR_CHRDEV_DELETE:
        cdev_del(&dev->cdev);
        /* fall through */
    case CHR_DEVICE_DESTROY:
        device_destroy(dev->cl, dev->devt);
        /* fall through */
    case CHR_CLASS_DESTROY:
        class_destroy(dev->cl);
        /* fall through */
    case CHR_UNREGISTER_CHRDEV:
        unregister_chrdev_region(dev->devt, 1);
        /* fall through */
    }    
}

/*----------------------------------------------------------------------------*/

static int chr_open(struct inode *inode, struct file *filp)
{
    struct chr_dev *dev;

    pr_debug("%s: open function called\n", DRIVER_NAME);

    dev = container_of(inode->i_cdev, struct chr_dev, cdev);
    filp->private_data = dev;
    
    /* create a separate session for each process */
    dev->session = kmalloc(sizeof (struct chr_session), GFP_KERNEL);
    if (IS_ERR(dev->session))
    {
        pr_err("%s: can't allocate memory for session\n", 
                DRIVER_NAME);
        return PTR_ERR(dev->session);
    }
    
    /* set default values */
    dev->session->byte_read     = 0;
    dev->session->byte_write    = 0;
    dev->session->buf_len       = 1024;
    
    /* get a unique session id */
    mutex_lock(&dev->chr_mutex);
    dev->session_count++;
    dev->session->session_id = dev->session_count;
    mutex_unlock(&dev->chr_mutex);

    return 0;
}

/*----------------------------------------------------------------------------*/

static int chr_release(struct inode *inode, struct file *filp)
{    
    struct chr_dev *dev;

    pr_debug("%s: release function called\n", DRIVER_NAME);

    dev = filp->private_data;
    
    /* increase the total number of read/written bytes */
    mutex_lock(&dev->chr_mutex);
    dev->total_byte_read += dev->session->byte_read;
    dev->total_byte_write += dev->session->byte_write;
    mutex_unlock(&dev->chr_mutex);
    
    /* show statistics */
    pr_debug("%s: current session id = %d\n", DRIVER_NAME, 
            dev->session->session_id);
    pr_debug("%s: bytes read = %ld\n", DRIVER_NAME, 
            dev->session->byte_read);
    pr_debug("%s: written bytes = %ld\n", DRIVER_NAME, 
            dev->session->byte_write);
    
    /* free allocated memmory */
    if (dev->session->buf)
        kfree(dev->session->buf);
        
    kfree(dev->session);

    return 0;
}

/*----------------------------------------------------------------------------*/

static ssize_t chr_read(struct file *filp, char __user *buf, 
                    size_t count, loff_t *pos)
{
    struct chr_dev *dev;
    size_t ret;    

    pr_debug("%s: read function called\n", DRIVER_NAME);

    dev = filp->private_data;
    
    /* check if buffer is empty */
    if (!dev->session->byte_write)
    {
        pr_warn("%s: buffer is empty\n", DRIVER_NAME);
        return -ENODATA;
    }
    
    /* at the end of file */
    if (*pos > dev->session->byte_write)
    {
        pr_debug("%s: at the end of file\n", DRIVER_NAME);
        return 0;
    }
    
    if (count > dev->session->byte_write)
    {
        pr_warn("%s: try to read {%ld} bytes when {%ld} actually exist\n",
            DRIVER_NAME, count, dev->session->byte_write);
        count = dev->session->byte_write;
    }
            
    ret = copy_to_user(buf, dev->session->buf, count);
    if (ret < 0)
    {
        pr_err("%s: can't read from local buffer\n", DRIVER_NAME);
        return ret;
    }
    
    dev->session->byte_read += count;
    *pos += count;
    
    return count;    
}

/*----------------------------------------------------------------------------*/

static ssize_t chr_write(struct file *filp, const char __user *buf, 
                        size_t count, loff_t *pos)
{
    struct chr_dev *dev;
    size_t ret;
    size_t max_len;

    pr_debug("%s: write function called\n", DRIVER_NAME);
    
    dev = filp->private_data;
    
    /* allocate memmory for buffer (do it here for more optimization)*/
    dev->session->buf = kmalloc(dev->session->buf_len, GFP_KERNEL);
    if (IS_ERR(dev->session->buf))
    {
        pr_err("%s: can't allocate memory for buffer\n", 
                DRIVER_NAME);
        return PTR_ERR(dev->session->buf);
    }
    
    /* check max free space in buffer */
    max_len = get_max_byte(&chrdev, count);
    if (!max_len)       /* buffer if full */
        return -ENOMEM;
        
    if (count > max_len)
    {
        pr_warn("%s: out of memory for buffer\n", DRIVER_NAME);
        count = max_len;
    }
    
    /* write data to buffer */
    ret = copy_from_user(dev->session->buf, buf, count);
    if (ret < 0)
    {
        pr_err("%s: can't write to local buffer\n", DRIVER_NAME);
        return ret;
    }
    
    dev->session->byte_write += count;
    *pos += count;
        
    return count;
}

/*----------------------------------------------------------------------------*/

static const struct file_operations fops =
{
    .owner          = THIS_MODULE,
    .open           = chr_open,
    .release        = chr_release,
    .read           = chr_read,
    .write          = chr_write,
};

/*----------------------------------------------------------------------------*/

static int __init chr_init(void)
{
    int ret = 0;
    
    pr_debug("%s: driver loaded\n", DRIVER_NAME);

    /*------------------------------------------------------------------------*/

    if (chr_major)
    {
        chrdev.devt = MKDEV(chr_major, chr_minor);
        ret = register_chrdev_region(chrdev.devt, 1, DRIVER_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&chrdev.devt, chr_minor, 1, DRIVER_NAME);
        chr_major = MAJOR(chrdev.devt);
    }

    if (ret < 0)
    {
        pr_debug("%s: can't register char device region\n", DRIVER_NAME);
        return ret;
    } 

    /*------------------------------------------------------------------------*/
    
    /* create class and device */
    chrdev.cl = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(chrdev.cl))
    {
        pr_err("%s: can't create class\n", DRIVER_NAME);
        cleanup_handler(&chrdev, CHR_UNREGISTER_CHRDEV);
        return PTR_ERR(chrdev.cl);
    }
    
    chrdev.device = device_create(chrdev.cl, NULL, chrdev.devt,
                                NULL, DRIVER_NAME);
    if (IS_ERR(chrdev.device))
    {
        pr_err("%s: can't create device\n", DRIVER_NAME);
        cleanup_handler(&chrdev, CHR_CLASS_DESTROY);
        return PTR_ERR(chrdev.device);
    }

    /*------------------------------------------------------------------------*/
    
    cdev_init(&chrdev.cdev, &fops);
    ret = cdev_add(&chrdev.cdev, chrdev.devt, 1);
    if (ret < 0)
    {
        pr_debug("%s: can't add char device to system\n", DRIVER_NAME);
        cleanup_handler(&chrdev, CHR_DEVICE_DESTROY);
        return ret;
    }

    /*------------------------------------------------------------------------*/

    /* set default values */
    chrdev.session_count    = 0;
    chrdev.total_byte_read  = 0;
    chrdev.total_byte_write = 0;
    chrdev.session          = NULL;
    mutex_init(&chrdev.chr_mutex);

    return 0;
}

/*----------------------------------------------------------------------------*/

static void __exit chr_exit(void)
{
    pr_debug("%s: total bytes read = %ld\n", DRIVER_NAME, 
            chrdev.total_byte_read);
    pr_debug("%s: total written bytes = %ld\n", DRIVER_NAME, 
            chrdev.total_byte_write);
    pr_debug("%s: session count = %d\n", DRIVER_NAME, 
            chrdev.session_count);
    cleanup_handler(&chrdev, CHR_CHRDEV_DELETE);
    pr_debug("%s: driver unloaded\n", DRIVER_NAME);
}

/*----------------------------------------------------------------------------*/

module_init(chr_init);
module_exit(chr_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Yustitskii <inst: yustitskii_kirill>");
MODULE_DESCRIPTION("Simple Linux char driver");
