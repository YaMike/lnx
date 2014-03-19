/*
 * =====================================================================================
 *
 *       Filename:  gpio_test.c
 *
 *    Description:  Test driver for gpio pins.
 *
 *        Version:  1.0
 *        Created:  03/18/2014 05:04:10 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Michael Likholet <m.likholet@ya.ru>
 *        Company:  i-Camp Engineering LLC
 *
 * =====================================================================================
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/device.h>

#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Likholet <m.likholet@ya.ru>");
MODULE_DESCRIPTION("Gpio test driver.");

#define DEV_NAME "gpio_test"
#define DEV_NAME_LEN 20

static ushort gpio_num = 0;

/*
 * Data structure and allocation routines.
 */

typedef struct {
	char 		name[DEV_NAME_LEN];
	dev_t 	devt;
	struct class *class;
	struct device *device;
} DevData;

typedef struct __test_data {
	DevData dev_data;
	int gpio_num;
} test_data;

static test_data *td = NULL;

static test_data * gpio_test_alloc(void) {
	if (gpio_num > 0) {
		test_data *t = kzalloc(sizeof(test_data), GFP_KERNEL);
		t->gpio_num = gpio_num;
		return t;
	} else {
		pr_err("GpioTest: Bad driver parameter!\n");
		return NULL;
	}
}

static void gpio_test_free(test_data ** t) {
	if (*t) {
		kfree(*t);
		*t = NULL;
	}
}

/*
 * devfs interface routines
 */
static int gpio_test_cdev_start(DevData *dev, struct file_operations *fops) {
	int ret = 0;
	int major;

	if ((major = register_chrdev(0, dev->name, fops)) < 0) {
		pr_alert("GpioTest:can't register chardev %d\n", major);
		return major;
	}

	dev->devt = MKDEV(major, 0);

	dev->class = class_create(THIS_MODULE, dev->name);
	if (IS_ERR_OR_NULL(dev->class))  {
		pr_alert("GpioTest: can't create class %s\n", dev->name);
		ret = PTR_ERR(dev->class);
		goto class_create_failed;
	}

	dev->device = device_create(dev->class, NULL, dev->devt, NULL, dev->name, NULL);
	if (IS_ERR_OR_NULL(dev->device)) {
		pr_alert("GpioTest: can't create device %s\n", dev->name);
		ret = PTR_ERR(dev->device);
		goto device_create_failed;
	}

	pr_info("GpioTest: char device %s (major %d) registered\n", dev->name, major);
	return 0;

device_create_failed:
	class_destroy(dev->class);
class_create_failed:
	unregister_chrdev(major, dev->name);

	return ret;
}

static void gpio_test_cdev_stop(DevData *dev) {
	pr_info("GpioTest: stop device.\n");
	device_destroy(dev->class, dev->devt);
	class_destroy(dev->class);
  unregister_chrdev(MAJOR(dev->devt), dev->name);
}

/*
 * File access methods 
 */
static int gpio_fo_open(struct inode *inode, struct file *filp) {
	pr_info("Gpio has been opened.\n");
	return 0;
}

ssize_t gpio_fo_read(struct file *filp, char __user *user_buff, size_t count, loff_t *offp) {
	int value = 0, missing = 0;
	pr_info("Read operation\n");
	pr_info("Value: %d\n", value = gpio_get_value(td->gpio_num));
	if (count < sizeof(int)) {
		pr_alert("GpioTest: not enough space provided by user.\n");
		return 0;
	}
	if (0 != (missing = copy_to_user(user_buff, &value, sizeof(int)))) {
		pr_alert("GpioTest: cannot copy to user %d bytes!\n", missing);
		return 0;
	}
	/* return count to make no trouble to user */
	return count;
}

ssize_t gpio_fo_write(struct file *filp, const char __user *user_buff, size_t count, loff_t *offp) {
	int flag, missing;
	pr_info("Write operation\n");
	if (0 != (missing = copy_from_user(&flag, user_buff, sizeof(int)))) {
		pr_alert("GpioTest: too much data has been passed! Missing %d bytes.\n", missing);
	}
	pr_info("GpioTest: flag = 0x%08X\n", flag & 0x1);

	gpio_set_value(td->gpio_num, flag & 0x1 ? 1 : 0);

	/* return count to make no trouble to user */
	return count;
}

int gpio_fo_release(struct inode *node, struct file *filp) {
	return 0;
}

static struct file_operations gpio_test_fops = {
  .owner = THIS_MODULE,
	.open = gpio_fo_open,
  .write = gpio_fo_write,
	.read = gpio_fo_read, 
	.release = gpio_fo_release,
  .llseek = no_llseek,
};

/*
 * init/exit
 */
static int __init gpio_test_init(void) {
	int err;
	pr_info("Starting gpio_test driver. Build: %s %s\n", __DATE__, __TIME__);
	if (NULL == (td = gpio_test_alloc())) {
		goto alloc_fail;
	}
	if (0 != (err = gpio_request(td->gpio_num, "GpioTest"))) {
		pr_alert("TestGpio driver: gpio reuest failed! Return status: %d\n", err);
		goto gpio_req_fail;
	}
	if (0 != (err = gpio_direction_output(td->gpio_num, 0))) {
		pr_alert("TestGpio driver: cannot setup direction! Return status: %d\n", err);
		goto gpio_req_fail;
	}
	strncpy(td->dev_data.name, DEV_NAME, DEV_NAME_LEN);
	if (0 != gpio_test_cdev_start(&td->dev_data, &gpio_test_fops)) {
		pr_alert("GpioTest: cannot create devfs interface.\n");
		goto devfs_fail;
	}

	return 0;

devfs_fail:
	gpio_test_cdev_stop(&td->dev_data);

gpio_req_fail:

	gpio_free(td->gpio_num);
	gpio_test_free(&td);

alloc_fail:
	td = NULL;

	return -1;
}

static void __exit gpio_test_exit(void) {
	pr_info("Exit gpio_test driver.\n");
	if (td != NULL) {
		gpio_test_cdev_stop(&td->dev_data);
		gpio_free(td->gpio_num);
		gpio_test_free(&td);
	}
	return;
}

module_init(gpio_test_init);
module_exit(gpio_test_exit);
module_param(gpio_num, ushort, 0444);
