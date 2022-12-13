#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/hashtable.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include "object.h"

// device names to be created and registered
#define DEVICE_NAME0        "ht438_dev_0"
#define DEVICE_NAME1        "ht438_dev_1"
#define MAX_DEVICES         2    

//structure for hastable node
typedef struct ht_obj_node{
	
	ht_object_t new_ht_obj;				/*structure of each hashtable node*/
	struct hlist_node next;			    /*pointer to the next node in the hashtable*/
}ht_obj_node, *Pht_obj_node;

//per device structure
 struct ht438_dev {
    //struct mutex dev_mutex;
	struct cdev cdev;               /* The cdev structure */
	char name[20];                   /* Name of device*/
    DECLARE_HASHTABLE(ht438_tbl, 5); 
    Pht_obj_node Pht_obj_node_new;                
} *ht438_devp[MAX_DEVICES];

static dev_t dev_number;      /* Allotted device number */
struct class *dev_class[MAX_DEVICES];          /* Tie with the device model */
static struct device *dev_device[MAX_DEVICES];

static char *user_name = "Assignment_3";

module_param(user_name,charp,0000);	//to get parameter from load.sh script to greet the user

int ht438_driver_open(struct inode *inode, struct file *file)
{
    struct ht438_dev *ht438_devp;
    ht438_devp = container_of(inode->i_cdev, struct ht438_dev, cdev);
    file->private_data = ht438_devp;
	printk("\n%s is openning \n", ht438_devp->name);
	return 0;
}

int ht438_driver_release(struct inode *inode, struct file *file)
{
    struct ht438_dev *ht438_devp = file->private_data;
	
	printk("\n%s is closing\n", ht438_devp->name);
	return 0;
}

ssize_t ht438_driver_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{   
    int bkt;
    bool update_flag1 = false, update_flag2 = false;

    struct ht438_dev *ht438_devp = file->private_data;

    Pht_obj_node pnew_node = NULL;
    Pht_obj_node ppnew_node = NULL;
    Pht_obj_node  temp = NULL;
    if(!(ppnew_node = kmalloc(sizeof(struct ht_obj_node), GFP_KERNEL)))
    {	
        printk("Bad Kmalloc\n");
		return -ENOMEM;
    }
    
    memset(ppnew_node, 0, sizeof(ht_obj_node));

    //mutex_lock(&ht438_devp->dev_mutex);
    if(copy_from_user(&ppnew_node->new_ht_obj,(ht_object_t *)buf, count))//sizeof(ht_object_t)
    {
        printk("Copy from user not possible\n");
        return -EFAULT;
    }

    ht438_devp->Pht_obj_node_new = ppnew_node;
	pnew_node = ht438_devp->Pht_obj_node_new;
    printk("Performing WRITE operation with:  KEY= %d,  DATA= %s\n", pnew_node->new_ht_obj.key, pnew_node->new_ht_obj.data);	

    if(hash_empty(ht438_devp->ht438_tbl))

	{
		hash_add(ht438_devp->ht438_tbl, &pnew_node->next, pnew_node->new_ht_obj.key);
		update_flag1 = true;
	}
    else
    {
        hash_for_each(ht438_devp->ht438_tbl, bkt, temp, next){
            if((temp->new_ht_obj.key == pnew_node->new_ht_obj.key) && (strcmp(pnew_node->new_ht_obj.data,"\0"))){
                hash_del(&temp->next);
                hash_add(ht438_devp->ht438_tbl, &pnew_node->next, pnew_node->new_ht_obj.key);
                update_flag2 = true;
            }
            else if(!(strcmp(pnew_node->new_ht_obj.data,"\0"))&&(temp->new_ht_obj.key == pnew_node->new_ht_obj.key))
            {
                hash_del(&temp->next);
                update_flag2 = true;
            }
        }
    }

    if((update_flag2== false) && (update_flag1 == false) && (strcmp(pnew_node->new_ht_obj.data,"\0")))
		 hash_add(ht438_devp->ht438_tbl, &pnew_node->next, pnew_node->new_ht_obj.key);

    //mutex_unlock(&ht438_devp->dev_mutex);
    return 0;
}

ssize_t ht438_driver_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
    bool item_found = false;
    ht_obj_node *temp;
    int i=0;

    struct ht438_dev *ht438_devp = file->private_data;
    ht_object_t temp_obj;

    //mutex_lock(&ht438_devp->dev_mutex);
    if(copy_from_user(&temp_obj, (ht_object_t *)buf, sizeof(ht_object_t)))//sizeof(ht_object_t)
    {
        printk("Copy from user not possible\n");
        return -EFAULT;
    }

    printk("Reading data from key: %d\n", temp_obj.key);

    hash_for_each_possible(ht438_devp->ht438_tbl, temp, next, temp_obj.key)
    {
        if(temp->new_ht_obj.key == temp_obj.key)
        {

            //strcpy(temp_obj.data, temp->new_ht_obj.data);
            for(i=0; i<4; i++)
            {
                temp_obj.data[i] = temp->new_ht_obj.data[i];
            }
            item_found = true;
        }
    }

    if(item_found)
    {
        printk("Data found at key : %d\n", temp_obj.key);
        if(copy_to_user((ht_object_t *)buf, &temp_obj, sizeof(ht_object_t)))
        {
           printk("Copy to user not possible\n");
           return -1;
        }

    }
    else{
        printk("Data not found\n");
        //errno = EINVAL;
        return -EINVAL;
    }
    //mutex_unlock(&ht438_devp->dev_mutex);

    return 0;
}

static struct file_operations ht438_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= ht438_driver_open,        /* Open method */
    .release	= ht438_driver_release,     /* Release method */
    .write		= ht438_driver_write,       /* Write method */
    .read		= ht438_driver_read,        /* Read method */
};

int __init ht438_driver_init(void)
{
    int ret, major, i=0;
    dev_t device;
    if (alloc_chrdev_region(&dev_number, 0, 2, "ht438_drv") < 0) {
		printk(KERN_DEBUG "Can't register device\n"); return -1;
	}
    dev_class[0] = class_create(THIS_MODULE, DEVICE_NAME0);
    dev_class[1] = class_create(THIS_MODULE, DEVICE_NAME1);

    ht438_devp[0] = kmalloc(sizeof(struct ht438_dev), GFP_KERNEL);
    ht438_devp[1] = kmalloc(sizeof(struct ht438_dev), GFP_KERNEL);

    if (!ht438_devp[0] || !ht438_devp[1]) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

    	/* Request I/O region */
	sprintf(ht438_devp[0]->name, DEVICE_NAME0);
    sprintf(ht438_devp[1]->name, DEVICE_NAME1);
	/* Connect the file operations with the cdev */
	cdev_init(&ht438_devp[0]->cdev, &ht438_fops);
    cdev_init(&ht438_devp[1]->cdev, &ht438_fops);

	ht438_devp[0]->cdev.owner = THIS_MODULE;
    ht438_devp[1]->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
    major = MAJOR(dev_number);
    for (i = 0; i<MAX_DEVICES; i++)
    {
        device = MKDEV(major, i);
        ret = cdev_add(&ht438_devp[i]->cdev, (device), 1);
        if (ret) {
            printk("Bad cdev\n");
            return ret;
        }

        if(i == 0)
        {
            dev_device[i] = device_create(dev_class[i], NULL, device, NULL, DEVICE_NAME0);
        }
        else{
            dev_device[i] = device_create(dev_class[i], NULL, device, NULL, DEVICE_NAME1);
        }
        hash_init(ht438_devp[i]->ht438_tbl);
        ht438_devp[i]->Pht_obj_node_new = NULL;
        //mutex_init(&ht438_devp[i]->dev_mutex);
    }

    printk("ht438 driver initialized.\n");
    return 0;
}

void __exit ht438_driver_exit(void)
{
	// device_remove_file(gmem_dev_device, &dev_attr_xxx);
	/* Release the major number */
    int i = 0;
    int buk;
	Pht_obj_node temp=NULL;
	unregister_chrdev_region((dev_number), 2);

	/* Destroy device */
    for(i =0; i< MAX_DEVICES;i++)
    {
        hash_for_each(ht438_devp[i]->ht438_tbl, buk, temp, next){           
        hash_del(&temp->next);}
        device_destroy (dev_class[i], MKDEV(MAJOR(dev_number), i));
        cdev_del(&ht438_devp[i]->cdev);
        kfree(ht438_devp[i]);
        /* Destroy driver_class */
        class_destroy(dev_class[i]);
    }
	printk("gmem driver removed.\n");
}

module_init(ht438_driver_init);
module_exit(ht438_driver_exit);
MODULE_LICENSE("GPL v2");