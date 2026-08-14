// SPDX-License-Identifier: GPL-2.0-only
/*
 * Stub /dev/iorap_dev node for the MediaTek/Xiaomi launch-boost iorap daemon.
 *
 * The iorapd shipped in the vendor com.android.profiling APEX treats the
 * absence of /dev/iorap_dev as a fatal lack of kernel support and exits,
 * which makes apexd revert the APEX and reboot the device.  The real driver
 * is proprietary and not present in any public kernel tree, so register a
 * minimal node that satisfies the open() gate.  Any other operation returns
 * -ENOTTY so the daemon degrades instead of dying.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>

#define IORAP_DEV_NAME		"iorap_dev"

static dev_t iorap_devno;
static struct cdev iorap_cdev;
static struct class *iorap_class;

static int iorap_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static long iorap_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

static char *iorap_devnode(struct device *dev, umode_t *mode)
{
	*mode = 0666;
	return NULL;
}

static const struct file_operations iorap_fops = {
	.owner		= THIS_MODULE,
	.open		= iorap_open,
	.unlocked_ioctl	= iorap_ioctl,
	.compat_ioctl	= iorap_ioctl,
};

static int __init iorap_init(void)
{
	struct device *dev;
	int err;

	err = alloc_chrdev_region(&iorap_devno, 0, 1, IORAP_DEV_NAME);
	if (err < 0)
		return err;

	cdev_init(&iorap_cdev, &iorap_fops);
	iorap_cdev.owner = THIS_MODULE;
	err = cdev_add(&iorap_cdev, iorap_devno, 1);
	if (err < 0)
		goto err_cdev;

	iorap_class = class_create(THIS_MODULE, IORAP_DEV_NAME);
	if (IS_ERR(iorap_class)) {
		err = PTR_ERR(iorap_class);
		goto err_class;
	}
	iorap_class->devnode = iorap_devnode;

	dev = device_create(iorap_class, NULL, iorap_devno, NULL, IORAP_DEV_NAME);
	if (IS_ERR(dev)) {
		err = PTR_ERR(dev);
		goto err_device;
	}

	pr_info("%s: stub device registered (maj %d)\n", IORAP_DEV_NAME,
		MAJOR(iorap_devno));
	return 0;

err_device:
	class_destroy(iorap_class);
err_class:
	cdev_del(&iorap_cdev);
err_cdev:
	unregister_chrdev_region(iorap_devno, 1);
	return err;
}

static void __exit iorap_exit(void)
{
	device_destroy(iorap_class, iorap_devno);
	class_destroy(iorap_class);
	cdev_del(&iorap_cdev);
	unregister_chrdev_region(iorap_devno, 1);
}

module_init(iorap_init);
module_exit(iorap_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Stub /dev/iorap_dev for launch-boost iorapd");
MODULE_AUTHOR("Youffx");
