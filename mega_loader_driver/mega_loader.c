#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>
#include <asm/bug.h>
#include <asm/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael Likholet m.likholet@ya.ru");
MODULE_DESCRIPTION("mega88 loader");

#define RST_GPIO 148
#define DEV_NAME_LEN 20
#define DEV_NAME "mega"

static ushort spi_mode=0;
static uint   speed=250000;

typedef struct {
	char 		name[DEV_NAME_LEN];
	dev_t 	devt;
	struct class *class;
	struct device *device;
} DevData;

typedef struct {
	struct spi_device *spi;
	DevData dev_data;
} LoaderData;

static LoaderData* p_LoaderData = NULL;

#define MEGA_PAGE_SIZE 32
#define MEGA_TX_SIZE 4
#define MEGA_RX_SIZE 4
#define MEGA_DELAY 50
#define MEGA_POLL_PERIOD_USEC 1000
#define MEGA_POLL_PERIOD_MSEC 50
#define MEGA_READY_WAIT_TO_MSECS 50000
#define MEGA_MEM_SIZE 8192
#define MEGA_TRY_CNT 5
#define MEGA_T_WD_FLASH 10 /* msecs */

struct tr_complete {
	struct spi_message *p_msg;
	char *rx_buf;
	char *tx_buf;
};

static void complete_func(void *context) {
	struct tr_complete *p_tr_complete = (struct tr_complete*)context;
	kfree(p_tr_complete->rx_buf);
	kfree(p_tr_complete->tx_buf);
	spi_message_free(p_tr_complete->p_msg);
}

static int wait_for_ready(void);

static int fw_send_page(unsigned short start_addr, char *fw) {
	char *tx_buf = kzalloc(MEGA_TX_SIZE*(2*MEGA_PAGE_SIZE+1), GFP_KERNEL);
	char *rx_buf = kzalloc(MEGA_RX_SIZE*(2*MEGA_PAGE_SIZE+1), GFP_KERNEL);
	struct tr_complete *p_tr_complete = kzalloc(sizeof(struct tr_complete), GFP_KERNEL);
	struct spi_message *p_msg = spi_message_alloc(2*MEGA_PAGE_SIZE+1, GFP_KERNEL);
	struct spi_transfer *p_trf = NULL;
	int ret = 0, trf_i = 0;

	if (!p_msg || !p_tr_complete) {
		pr_alert("Loader: not enough memory for spi message!\n");
		return -ENOMEM;
	}

	p_tr_complete->rx_buf = rx_buf;
	p_tr_complete->tx_buf = tx_buf;
	p_tr_complete->p_msg = p_msg;

	p_msg->context = (void*)p_tr_complete;
	p_msg->complete = complete_func;

	list_for_each_entry(p_trf, &p_msg->transfers, transfer_list) {
		p_trf->len = 4;
		p_trf->rx_buf = &rx_buf[MEGA_RX_SIZE*trf_i];
		p_trf->tx_buf = &tx_buf[MEGA_TX_SIZE*trf_i];
		if (2*MEGA_PAGE_SIZE == trf_i) {
			((char*)p_trf->tx_buf)[0] = 0x4C;
			((char*)p_trf->tx_buf)[2] = (char)(start_addr & 0xff);
			((char*)p_trf->tx_buf)[1] = (char)((start_addr >> 8) & 0xff);
			((char*)p_trf->tx_buf)[3] = 0x00;
		} else {
			((char*)p_trf->tx_buf)[0] = trf_i % 2 ? 0x48 : 0x40;
			((char*)p_trf->tx_buf)[1] = 0x00;
			((char*)p_trf->tx_buf)[2] = trf_i >> 1;
			((char*)p_trf->tx_buf)[3] = *fw++;
		}
		//pr_info("Loader: cmd: %02X%02X%02X%02X\n", ((char*)p_trf->tx_buf)[0], ((char*)p_trf->tx_buf)[1], ((char*)p_trf->tx_buf)[2], ((char*)p_trf->tx_buf)[3]);
		p_trf->cs_change = trf_i == 2*MEGA_PAGE_SIZE ? 0 : 1;
		trf_i++;
	}
	if (NULL == p_LoaderData->spi) {
		ret = -ESHUTDOWN;
	} else {
		/*ret = spi_sync(p_LoaderData->spi, p_msg);*/
		ret = spi_async(p_LoaderData->spi, p_msg);
	}
	/*
	 *spi_message_free(p_msg);
	 *kfree(tx_buf);
	 *kfree(rx_buf);
	 */
	return ret;
}

static int fw_send(unsigned short start_addr, int size, char *fw) {
	int page_count = size/(MEGA_PAGE_SIZE*2), page,
			bytes_send = 0, ret;
	if (size < MEGA_PAGE_SIZE) {
		return -ENODATA;
	}
	for (page = 0; page < page_count; page++) {
		if (0 > (ret = fw_send_page(start_addr+MEGA_PAGE_SIZE*page, fw + 2*MEGA_PAGE_SIZE*page))) {
			pr_alert("Error while sending firmware: %d.\n", ret);
			return ret;
		} else {
			bytes_send += 2*MEGA_PAGE_SIZE;
			pr_info("Loader: 0x%04X: %d bytes written (this transaction total %d).\n", start_addr+MEGA_PAGE_SIZE*page, 2*MEGA_PAGE_SIZE, bytes_send);
			if (!wait_for_ready()) {
				pr_warn("Loader: chip doesn't become ready after flashing %d page! Terminating.\n", page);
				return 0;
			}
			/*msleep(MEGA_T_WD_FLASH);*/
		}
	}
	return bytes_send;
}

static int fw_recv_page(unsigned short start_addr, char *fw) {
	char *tx_buf = kzalloc(MEGA_TX_SIZE*(2*MEGA_PAGE_SIZE), GFP_KERNEL);
	char *rx_buf = kzalloc(MEGA_RX_SIZE*(2*MEGA_PAGE_SIZE), GFP_KERNEL);
	struct spi_message *p_msg = spi_message_alloc(2*MEGA_PAGE_SIZE, GFP_KERNEL);
	struct spi_transfer *p_trf = NULL;
	int ret = 0, trf_i = 0;
	char *fw_ptr = fw;

	if (!p_msg) {
		pr_alert("Loader: not enough memory for spi message!\n");
		return -ENOMEM;
	}
	mdelay(MEGA_DELAY);
	list_for_each_entry(p_trf, &p_msg->transfers, transfer_list) {
		p_trf->len = 4;
		p_trf->rx_buf = &rx_buf[MEGA_RX_SIZE*trf_i];
		p_trf->tx_buf = &tx_buf[MEGA_TX_SIZE*trf_i];
		((char*)p_trf->tx_buf)[0] = trf_i % 2 ? 0x28 : 0x20;
		((char*)p_trf->tx_buf)[1] = (char)((start_addr >> 8) & 0xff);
		/*((char*)p_trf->tx_buf)[2] = (char)(start_addr & 0xff);*/
		((char*)p_trf->tx_buf)[2] = (start_addr&0xe0)|((trf_i >> 1)&0x1f);
		((char*)p_trf->tx_buf)[3] = *fw_ptr++;
		//pr_info("Loader: read cmd: %02X%02X%02X%02X\n", ((char*)p_trf->tx_buf)[0], ((char*)p_trf->tx_buf)[1], ((char*)p_trf->tx_buf)[2], ((char*)p_trf->tx_buf)[3]);
		p_trf->cs_change = trf_i == 2*MEGA_PAGE_SIZE ? 0 : 1;
		//p_trf->delay_usecs = MEGA_DELAY;
		trf_i++;
	}
	if (NULL == p_LoaderData->spi) {
		ret = -ESHUTDOWN;
	} else {
		ret = spi_sync(p_LoaderData->spi, p_msg);
		/*ret = spi_async(p_LoaderData->spi, p_msg);*/
	}
	list_for_each_entry(p_trf, &p_msg->transfers, transfer_list) {
		*fw++ = ((char*)p_trf->rx_buf)[3];
		//pr_info("Loader: result: %02X", fw[trf_i-1]);
	}
	spi_message_free(p_msg);
	kfree(tx_buf);
	kfree(rx_buf);
	return ret;
}

static int fw_read(unsigned short start_addr, int size, char *fw) {
	int page, page_count,
			bytes_recv = 0, ret;

	if (size < MEGA_PAGE_SIZE ||
			start_addr > MEGA_MEM_SIZE) {
		return -ENODATA;
	}
	page_count = (size + MEGA_PAGE_SIZE*2-1)/(MEGA_PAGE_SIZE*2);
	for (page = 0; page < page_count; page++) {
		if (0 > (ret = fw_recv_page(start_addr+MEGA_PAGE_SIZE*page, &fw[2*MEGA_PAGE_SIZE*page]))) {
			pr_alert("Error while receiving firmware: %d.\n", ret);
			return ret;
		} else {
			bytes_recv += 2*MEGA_PAGE_SIZE;
			if (bytes_recv > size) {
				bytes_recv = size;
			}
			pr_info("Loader: 0x%04X: %d bytes received (this transaction total %d).\n", start_addr+MEGA_PAGE_SIZE*page, 2*MEGA_PAGE_SIZE, bytes_recv);
		}
	}
	return bytes_recv;
}

static int cdev_start(DevData *dev, struct file_operations *fops) {
	int ret = 0;
	int major;

	if ((major = register_chrdev(0, dev->name, fops)) < 0) {
		pr_alert("Loader:can't register chardev %d\n", major);
		return major;
	}

	dev->devt = MKDEV(major, 0);

	dev->class = class_create(THIS_MODULE, dev->name);
	if (IS_ERR_OR_NULL(dev->class))  {
		pr_alert("Loader: can't create class %s\n", dev->name);
		ret = PTR_ERR(dev->class);
		goto class_create_failed;
	}

	dev->device = device_create(dev->class, NULL, dev->devt, NULL, dev->name);
	if (IS_ERR_OR_NULL(dev->device)) {
		pr_alert("Loader: can't create device %s\n", dev->name);
		ret = PTR_ERR(dev->device);
		goto device_create_failed;
	}

	pr_info("Loader: char device %s (major %d) registered\n", dev->name, major);

	return 0;

device_create_failed:
	class_destroy(dev->class);
class_create_failed:
	unregister_chrdev(major, dev->name);

	return ret;
}

static void cdev_stop(DevData *dev) {
	device_destroy(dev->class, dev->devt);
	class_destroy(dev->class);
  unregister_chrdev(MAJOR(dev->devt), dev->name);
}

static int firmware_bytes = 0;
static int firmware_addr = 0;
static int remain_fw_size = 0;
static char *remain_fw = NULL;
static int open_count = 0;

static void reset_positive_pulse(void) {
	gpio_set_value(RST_GPIO, 0);
	udelay(100);
	gpio_set_value(RST_GPIO, 1);
	udelay(100);
	gpio_set_value(RST_GPIO, 0);
	mdelay(50);
}

static int loader_open(struct inode *inode, struct file *filp) {
	if (open_count > 0) {
		return -EBUSY;
	} else {
		open_count = 1;
	}
	firmware_bytes = 0;
	firmware_addr = 0;
	remain_fw_size = 0;
  return 0;
}

static int ready = 0;

static void poll_complete_func(void *context) {
	struct tr_complete *p_tr_complete = (struct tr_complete*)context;
	if (0x00 == (p_tr_complete->rx_buf[3] & 0x01)) {
		ready = 1;
	}
	complete_func(context);
}

static void ask_chip_for_ready(void) {
	struct tr_complete *p_tr_complete = kzalloc(sizeof(struct tr_complete), GFP_KERNEL);
	struct spi_message *p_msg = spi_message_alloc(1, GFP_KERNEL);
	struct spi_transfer *p_transf;
	unsigned char *tx_buf = kzalloc(sizeof(char[4]), GFP_KERNEL);
	unsigned char *rx_buf = kzalloc(sizeof(char[4]), GFP_KERNEL);

	tx_buf[0] = 0xF0;
	tx_buf[1] = tx_buf[2] = tx_buf[3] = 0;
	rx_buf[0] = rx_buf[1] = rx_buf[2] = rx_buf[3] = 0;

	p_tr_complete->rx_buf = rx_buf;
	p_tr_complete->tx_buf = tx_buf;
	p_tr_complete->p_msg = p_msg;

	p_msg->context = (void*)p_tr_complete;
	p_msg->complete = poll_complete_func;		

	list_for_each_entry(p_transf, &p_msg->transfers, transfer_list) {
		p_transf->rx_buf = rx_buf;
		p_transf->tx_buf = tx_buf;
		p_transf->len = 4;
	}

	spi_async(p_LoaderData->spi, p_msg);
}

static int wait_for_ready(void) {
	int counter = 0;
	ready = 0;
	while (true) {
		if (ready) {
			/*pr_info("Not ready %d times\n", counter);*/
			/*pr_info("Loader: ready!\n");*/
			break;
		}
		if (counter > 5000) {
			pr_info("Not ready %d times. Terminating.\n", counter);
			break;
		}
		ask_chip_for_ready();
		mdelay(MEGA_POLL_PERIOD_MSEC);
		counter++;
	}
	mdelay(1);
	return ready;
}

static void chip_spi_sync_cmd(char cmd[4], char answer[4]) {
	struct spi_message *p_msg;
	struct spi_transfer *p_transf;
	int ret = 0;
	p_msg = kzalloc(sizeof(struct spi_message), GFP_KERNEL);
	p_transf = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	spi_message_init(p_msg);
	p_transf->rx_buf = &answer[0];
	p_transf->tx_buf = &cmd[0];
	p_transf->len = 4;
	spi_message_add_tail(p_transf, p_msg);
	ret = spi_sync(p_LoaderData->spi, p_msg);
	kfree(p_msg);
	kfree(p_transf);
}

static int chip_erase(void) {
	unsigned char tx_buf[4] = { 0xAC, 0x80, 0x00, 0x00 };
	unsigned char rx_buf[4] = { 0 };
	pr_warn("Loader: ask for chip erase\n");
	(void)chip_spi_sync_cmd(tx_buf, rx_buf);
	pr_warn("Loader: chip erased.\n");
	return 0;
}

static unsigned char read_signature(unsigned char sign_byte) {
	unsigned char tx_buf[4] = { 0x30, 0x00, sign_byte, 0x00 };
	unsigned char rx_buf[4] = { 0 };
	(void)chip_spi_sync_cmd(tx_buf, rx_buf);
	pr_info("Loader: ask for signature: %02X %02X %02X %02X.\n", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3]);
	pr_info("Loader: signature: %02X %02X %02X %02X.\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3]);
	return rx_buf[3];
}

DEFINE_SEMAPHORE(sema_write);

static ssize_t loader_write(struct file *filp, const char __user *user_buff, size_t count, loff_t *offp) {
  int missing, bytes_send;
	char *pc_firmware_part = NULL;
	
	if (down_interruptible(&sema_write)) {
		return -ERESTARTSYS;
	}
	pr_info("User writes %d bytes.\n", count);
	
	pc_firmware_part = kzalloc(count + remain_fw_size, GFP_KERNEL);
	if (!pc_firmware_part) {
		pr_alert("Loader: cannot alloc memory!\n");
		up(&sema_write);
		return -ENOMEM;
	}
	if (remain_fw_size > 0) {
		if (remain_fw != NULL) {
			memcpy(pc_firmware_part, remain_fw, remain_fw_size);
			if (remain_fw) {
				kfree(remain_fw);
			}
		} else {
			pr_alert("Loader: remain buffer corrupted!\n");
			remain_fw_size = 0;
		}
	}
  missing = copy_from_user(pc_firmware_part + remain_fw_size, user_buff, count);
  if (missing != 0) {
    pr_alert("Loader: too much data were passed to loader device! Missing %d bytes.\n", missing);
		up(&sema_write);
		return -EAGAIN;
  }

	if (firmware_bytes == 0) {
		char tx_buf[4] = { 0xAC, 0x53, 0x00, 0x00 };
		char rx_buf[4] = { 0 };
		int tries = MEGA_TRY_CNT;

		/* program mode request */
		while (tries--) {
			reset_positive_pulse();
			(void)chip_spi_sync_cmd(tx_buf, rx_buf);

			if (rx_buf[2] != 0x53) {
				pr_warn("Loader: mega88 gave us bad response: %02X %02X %02X %02X. Trying again (try #%d)...\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], MEGA_TRY_CNT - tries - 1);
				mdelay(100);
			} else {
				/* read signature bits */
				(void)read_signature(0x00);
				(void)read_signature(0x01);
				(void)read_signature(0x02);
				break;
			}
		}
		if (tries < 0) {
			pr_warn("Loader: mega88 doesn't give a right response. Terminating.\n");
			up(&sema_write);
			return -ERESTARTSYS;
		}
	}
	chip_erase();
	if (!wait_for_ready()) {
		pr_warn("Loader: chip is not ready!\n");
		return -EBUSY;
	} else {
		pr_info("Loader: chip erased and ready.\n");
	}

	if (count < 64) {
		pr_warn("Loader: need at least one page to program memory!\n");
		up(&sema_write);
		return -ENODATA;
	}
	pr_info("Loader: request to send firmware was successful. Ending.\n");
	bytes_send = fw_send(firmware_addr, count + remain_fw_size, pc_firmware_part);
	if (bytes_send < count + remain_fw_size) {
		int bytes_to_store = count + remain_fw_size - bytes_send;
		//pr_info("Loader: bytes send: %d, bytes remains: %d\n", bytes_send, bytes_to_store);
		remain_fw = kzalloc(bytes_to_store, GFP_KERNEL);
		memcpy(remain_fw, pc_firmware_part + bytes_send, bytes_to_store);
		remain_fw_size = bytes_to_store;
	} else {
		remain_fw_size = 0;
	}
	firmware_addr += bytes_send/2;
	firmware_bytes += bytes_send;
	pr_info("Loader: partial complete.\n");
	up(&sema_write);
  return count;
}

int loader_release(struct inode *node, struct file *filp) {
	int ret;
	if (remain_fw_size > 0) {
		int fw_size = 2*MEGA_PAGE_SIZE*((int)((remain_fw_size + 2*MEGA_PAGE_SIZE)/(2*MEGA_PAGE_SIZE)));
		char *fw = kzalloc(fw_size, GFP_KERNEL);
		memset(fw, 0xff, fw_size);
		memcpy(fw, remain_fw, remain_fw_size);
		ret = fw_send(firmware_addr, fw_size, fw);
		if (ret < fw_size) {
			pr_alert("Loader: cannot write remain data (%d bytes) at close! Ret = %d.\n", remain_fw_size, ret);
		} else {
			firmware_bytes += remain_fw_size;
		}
		kfree(remain_fw);
	} else {
		if (remain_fw) {
			pr_alert("Loader: remain fw buffer corrupted after mcu reflashing!\n");
			kfree(remain_fw);
		}
	}
	mdelay(100);
	gpio_set_value(RST_GPIO, 1);
	pr_info("Loader: finished. %d bytes written.\n", firmware_bytes);
	remain_fw_size = 0;
	firmware_bytes = 0;
	firmware_addr = 0;
	remain_fw = NULL;
	open_count = 0;
	return 0;
}

DEFINE_SEMAPHORE(sema_read);

ssize_t loader_read(struct file *filp, char __user *user_buff, size_t count, loff_t *offp) {
	int bytes_recv = 0;
	char *firmware = NULL;
	if (down_interruptible(&sema_read)) {
		pr_alert("Loader: reading interrupted!\n");
		return -EINTR;
	}
	if (firmware_bytes >= MEGA_MEM_SIZE) {
		return 0;
	}
	if (firmware_bytes + count > MEGA_MEM_SIZE) {
		count = MEGA_MEM_SIZE - firmware_bytes;
	}
	if (firmware_bytes == 0) {
		char tx_buf[4] = { 0xAC, 0x53, 0x00, 0x00 };
		char rx_buf[4] = { 0 };
		int tries = MEGA_TRY_CNT;
		/* program mode request */
		while (tries--) {
			reset_positive_pulse();
			(void)chip_spi_sync_cmd(tx_buf, rx_buf);

			if (rx_buf[2] != 0x53) {
				pr_warn("Loader: mega88 gave us bad response: %02X %02X %02X %02X. Trying again (try #%d)...\n", rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], MEGA_TRY_CNT - tries - 1);
				mdelay(100);
			} else {
				break;
			}
		}
		if (tries < 0) {
			pr_warn("Loader: reader: mega88 doesn't give a right response. Terminating.\n");
			up(&sema_read);
			return -ERESTARTSYS;
		}
	}
	firmware = kzalloc(MEGA_PAGE_SIZE*((count+2*MEGA_PAGE_SIZE)/MEGA_PAGE_SIZE)*sizeof(char), GFP_KERNEL);
	pr_info("Loader: start address = %X\n", firmware_addr);
	if (0 > (bytes_recv = fw_read(firmware_addr, count, firmware))) {
		pr_alert("Loader: error while reading: %d\n", count);
	} else {
		int missing = 0;
		missing = copy_to_user(user_buff, firmware, bytes_recv);
		BUG_ON(missing > 0);
		pr_info("Loader: finising copying from addr 0x%X. Missing %d bytes.\n", firmware_addr, missing);
		firmware_addr += bytes_recv/2;
		firmware_bytes += bytes_recv;
	}
	kfree(firmware);
	up(&sema_read);
	return bytes_recv;
}

static struct file_operations loader_fops = {
  .owner = THIS_MODULE,
	.open = loader_open,
  .write = loader_write,
	.read = loader_read, 
	.release = loader_release,
  .llseek = no_llseek,
};

static int __devinit loader_probe(struct spi_device *spi_dev) {
  int err = 0;

  /* allocate data structure */
  if (NULL == (p_LoaderData = (LoaderData*)kmalloc(sizeof(LoaderData), GFP_KERNEL))) {
    pr_alert("Loader: cannot allocate memory!\n");
    return -ENOMEM;
  }

  /* get and adjust gpio */
  err = gpio_request(RST_GPIO, "atmega interrupt");
  if (err != 0) 
  {
    pr_alert("Loader: gpio request failed! Return status: %d\n", err);
    goto gpio_fail;
  }

  if (0 != (err = gpio_direction_output(RST_GPIO,1))) {
    pr_alert("Loader: cannot set direction for gpio! Return status: %d\n", err);
    goto gpio_dir_fail;
  }

  /* screen managment data */
	strcpy(&p_LoaderData->dev_data.name[0], DEV_NAME);
  if (0 > (err = cdev_start(&p_LoaderData->dev_data, &loader_fops))) {
    pr_alert("Loader: error while creating screen manager device. Return status %d\n", err);
    goto dev_create_fail;
  }

	spi_dev->mode = spi_mode;
	spi_dev->max_speed_hz = speed;
	if (0 != (err = spi_setup(spi_dev))) {
		pr_alert("Loader: cannot setup spi! Error: %d\n", err);
		goto setup_spi_fail;
	}
	p_LoaderData->spi = spi_dev;

  return 0;
setup_spi_fail:
dev_create_fail:
	cdev_stop(&p_LoaderData->dev_data);

gpio_dir_fail:
  gpio_free(RST_GPIO);

gpio_fail:
  kfree(p_LoaderData);

  return err;
}

static int __devexit loader_remove(struct spi_device *spi) {
  if (p_LoaderData == NULL) {
    pr_alert("Loader: cannot get driver data at loader remove routine!\n");
  }
	gpio_set_value(RST_GPIO, 1);
  gpio_free(RST_GPIO);
	cdev_stop(&p_LoaderData->dev_data);
	kfree(p_LoaderData);
  return 0;
}

static struct spi_driver __refdata spi_loader_driver = {
	.driver = {
		.name   = "mega_loader",
		.owner  = THIS_MODULE,
	},
	.probe  = loader_probe,
	.remove = __devexit_p(loader_remove),
};

static int __init loader_init(void) {
  pr_info("loader: build %s %s\n", __DATE__, __TIME__);
  return spi_register_driver(&spi_loader_driver);
}

static void __exit loader_exit(void) {
  pr_info("loader: exit...");
  return spi_unregister_driver(&spi_loader_driver);
}

module_init(loader_init);
module_exit(loader_exit);
module_param(spi_mode, ushort, 0444);
module_param(speed, uint, 0444);
