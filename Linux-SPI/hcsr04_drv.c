
/*--------------------------------HCSRO4 driver-------------------------------------------

Basic Driver for HCSR04 ultrasonic sensor

-----------------------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

// device names to be created and registered
#define DEVICE_NAME        "hcsr04"

//Define the ioctl code
#define SET_TRIGGER _IOW('a', 'a', int *)
#define SET_ECHO	_IOW('a','b', int *)

unsigned int irq_num;
ktime_t start, end;
int measure_in_progress=0, device_busy=0;
/* per device structure */
struct hcsr04_dev {
	struct mutex mutex;
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/
	unsigned long distance;
    struct gpio_desc *trigger, *echo;
} *hcsr04_devp;

static dev_t hcsr04_dev_number;      /* Allotted device number */
struct class *hcsr04_dev_class;          /* Tie with the device model */
static struct device *hcsr04_dev_device;

static char *user_name = "Assignment_4";
int trigger_pin = 0, echo_pin = 0;

module_param(user_name,charp,0000);	//to get parameter from load.sh script to greet the user

//Routine for interrurpt handling
static irqreturn_t interrupt_handler(int irq, void *device)
{
	struct hcsr04_dev *hcsr04_devp = (struct hcsr04_dev *)device;

	//Lock the mutex
	mutex_lock(&hcsr04_devp->mutex);

	if(device_busy)
	{
        // If to check rising or falling edge
        if (!measure_in_progress) {
            start = ktime_get();
            measure_in_progress = 1;
        }
        else {
            end = ktime_get();
            hcsr04_devp->distance = (unsigned long)(end - start);
            // Input pulse scaling
            hcsr04_devp->distance /= 10;
            if (hcsr04_devp->distance < 10000) {
                hcsr04_devp->distance = 10000;
            }
            else if (hcsr04_devp->distance > 1000000) {
                hcsr04_devp->distance = 1000000;
            }
            measure_in_progress = 0;
            device_busy = 0;
        }		
	}	
	mutex_unlock(&hcsr04_devp->mutex);
    return IRQ_HANDLED;
}

//Open Driver
int hcsr04_driver_open(struct inode *inode, struct file *file)
{
	struct hcsr04_dev *hcsr04_devp;
	printk("\nopening\n");

	/* Get the per-device structure that contains this cdev */
	hcsr04_devp = container_of(inode->i_cdev, struct hcsr04_dev, cdev);
	
	//Set initial distance to 0.
	hcsr04_devp->distance = 0;

	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = hcsr04_devp;
	printk("\n%s is openning \n", hcsr04_devp->name);
	return 0;
}

//Release the driver
int hcsr04_driver_release(struct inode *inode, struct file *file)
{
	struct hcsr04_dev *hcsr04_devp = file->private_data;

	//Free the IRQs and GPIOs
	if(hcsr04_devp->echo)
	{
		gpiod_put(hcsr04_devp->echo);
	}
	if(hcsr04_devp->trigger)
	{
		gpiod_put(hcsr04_devp->trigger);
	}
	
	free_irq(irq_num, hcsr04_devp);
	printk("\n%s is closing\n", hcsr04_devp->name);
	
	return 0;
}

//Function for driver ioctl
static long hcsr04_driver_ioctl(struct file *file, unsigned int cmd, unsigned long param)
{

	struct hcsr04_dev *hcsr04_devp = file->private_data;
	int ret = 0;
	switch(cmd)
	{
		case SET_TRIGGER:
			if(copy_from_user(&trigger_pin,(int *)param, sizeof(trigger_pin)))
    		{
        		printk(KERN_INFO "Copy from user not possible\n");
        		return -EFAULT;
    		}
			hcsr04_devp->trigger = gpio_to_desc(trigger_pin);
			if(!hcsr04_devp->trigger)
			{
				printk(KERN_DEBUG "Invalid GPIO for trigger\n");
				return -1;
			}
			printk(KERN_INFO "trigger pin is set\n");
			ret = gpiod_direction_output(hcsr04_devp->trigger, 0);
			if(ret)
			{
				printk(KERN_INFO "Can't set direction on trigger pin\n");
				return -EFAULT;
			}
			break;
		case SET_ECHO:
			if(copy_from_user(&echo_pin,(int *)param, sizeof(echo_pin)))
    		{
        		printk(KERN_INFO "Copy from user not possible\n");
        		return -EFAULT;
    		}
			hcsr04_devp->echo = gpio_to_desc(echo_pin);
			if(!hcsr04_devp->echo)
			{
				printk(KERN_DEBUG "Invalid GPIO for echo\n");
				return -1;
			}
			printk(KERN_INFO "echo pin is set\n");
			ret = gpiod_direction_input(hcsr04_devp->echo);
			if (ret)
			{
				printk(KERN_INFO "Can't set direction on echo pin\n");
				return -EFAULT;				
			}
			irq_num = gpiod_to_irq(hcsr04_devp->echo);
			if(irq_num<0)
			{
				printk(KERN_INFO "Error converting GPIO to IRQ\n");
				return -EFAULT;
			}
			ret = request_irq(irq_num, interrupt_handler, (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), "HCSR-04 Echo",hcsr04_devp);
			break;
		default:
			printk(KERN_DEBUG "Invalid command\n");
			break;
	}
	return 0;
}

//Function for device read
ssize_t hcsr04_driver_read(struct file* file, char* buf,
    size_t count, loff_t* ppos)
{
    struct hcsr04_dev *hcsr04_devp = file->private_data;

    // Copy the saved distance to the distance buffer in user space
    if (copy_to_user((unsigned long*)buf, &hcsr04_devp->distance, sizeof(unsigned long))) {
        printk(KERN_INFO "Cannot copy to user\n");
        return -1;
    }

    return 0;
}

//Fuction for driver write
ssize_t hcsr04_driver_write(struct file* file, const char* buf,
    size_t count, loff_t* ppos)
{
    struct hcsr04_dev *hcsr04_devp = file->private_data;

	if(!hcsr04_devp->trigger || !hcsr04_devp->echo)
	{
		printk(KERN_INFO "Trigger or Echo pin is not set\n");
		return -EINVAL;
	}
    // Check If device is busy
    if (!device_busy) {
        // Lock the mutex before sending Trigger Pulse
        mutex_lock(&hcsr04_devp->mutex);

        // Send 10 us pulse on trigger pin
        gpiod_set_value(hcsr04_devp->trigger, 1);
        udelay(10);
        gpiod_set_value(hcsr04_devp->trigger, 0);
        device_busy = 1; //set device_busy

        // mutex unlock
        mutex_unlock(&hcsr04_devp->mutex);
    }
    else {
        printk(KERN_INFO "Device Busy\n");
		return -EBUSY;
    }

    return 0;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations hcsr04_fops = {
    .owner					= THIS_MODULE,           	 /* Owner */
    .open					= hcsr04_driver_open,        /* Open method */
    .release				= hcsr04_driver_release,     /* Release method */
    .write					= hcsr04_driver_write,       /* Write method */
    .read					= hcsr04_driver_read,        /* Read method */
	.unlocked_ioctl 		= hcsr04_driver_ioctl,		/* Ioctl method */
};


int __init hcsr04_driver_init(void)
{
    int ret;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&hcsr04_dev_number, 0, 1, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;	}

	/* Populate sysfs entries */
	hcsr04_dev_class = class_create(THIS_MODULE, DEVICE_NAME);   

	/* Allocate memory for the per-device structure */
	hcsr04_devp = kmalloc(sizeof(struct hcsr04_dev), GFP_KERNEL);   

	if (!hcsr04_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}  

	/* Request I/O region */
	sprintf(hcsr04_devp->name, DEVICE_NAME);

	cdev_init(&hcsr04_devp->cdev, &hcsr04_fops);
	hcsr04_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&hcsr04_devp->cdev, (hcsr04_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* create device */
	hcsr04_dev_device = device_create(hcsr04_dev_class, NULL, MKDEV(MAJOR(hcsr04_dev_number), 0), NULL, DEVICE_NAME);	
	//Initialize descriptor to NULL
	hcsr04_devp->trigger = NULL;
	hcsr04_devp->echo = NULL;
	mutex_init(&hcsr04_devp->mutex);
	printk("hcsr04 driver created.\n");
    return 0;
}

void __exit hcsr04_driver_exit(void)
{
	/* Release the major number */
	unregister_chrdev_region((hcsr04_dev_number), 1);

	/* Destroy device */
	device_destroy (hcsr04_dev_class, MKDEV(MAJOR(hcsr04_dev_number), 0));
	cdev_del(&hcsr04_devp->cdev);
	kfree(hcsr04_devp);
	
	/* Destroy driver_class */
	class_destroy(hcsr04_dev_class);

	printk("hcsr04 driver removed.\n");
}

module_init(hcsr04_driver_init);
module_exit(hcsr04_driver_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Roshan Raj");
MODULE_DESCRIPTION("Registers a character device to implement HCSR-04 sensor operations");