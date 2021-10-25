/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"

#include <linux/slab.h> // kmalloc
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Dhruv"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
	
    PDEBUG("open");
	/**
	 * TODO: handle open
	 */

    /* Obtain aesd device info from inode and cdev */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);

    /* Store the dev obtained into the filp->private_data */
    filp->private_data = dev;
	//PDEBUG("devptr = %p", (void *)dev);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
    struct aesd_dev *dev = (struct aesd_dev *)(filp->private_data);
    char *kbuf = NULL;
    struct aesd_buffer_entry *buffer_read_last;
    size_t offset_byte;
    unsigned long ctu_return;
    size_t char_offset;
    size_t out_offs_count = 0;
    size_t total_size = dev->aesd_circ_buffer.entry[dev->aesd_circ_buffer.out_offs].size;
    size_t buf_size = *f_pos;

	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */
    
    /* Restrict reads to only the size of a single aesd_buffer_entry */
    if(count > (dev->aesd_circ_buffer.entry[dev->aesd_circ_buffer.out_offs].size))
        count = dev->aesd_circ_buffer.entry[dev->aesd_circ_buffer.out_offs].size;

    while(*f_pos + 1 >= total_size)
    {
        if(out_offs_count == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1)
        {
            retval = 0;
            return retval;
        }
        out_offs_count++;
        PDEBUG("Total_size = %ld", total_size);
        total_size += dev->aesd_circ_buffer.entry[(dev->aesd_circ_buffer.out_offs) + out_offs_count].size;

    }

    char_offset = total_size - 1;
    if(char_offset == -1) char_offset = 0;


    /* Get the aesd_buffer_entry ptr and the offset byte for fpos */
     buffer_read_last = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->aesd_circ_buffer),
     char_offset, &offset_byte);

    if(buffer_read_last == NULL)    
     PDEBUG("char0ff = %ld, offfbyte = %ld",char_offset, offset_byte);
	
    if(buffer_read_last == NULL)
        return 0;

    /* Update fpos and kbuf ptr */
    *f_pos += offset_byte;
    kbuf = (char *)buffer_read_last->buffptr;
    buf_size = *f_pos - buf_size + 1;

    if(kbuf == NULL) return 0;

    /* Copy kbuf to user-space buffer */
    ctu_return = copy_to_user(buf, kbuf, buf_size);
    if(ctu_return != 0)
        PDEBUG("copy_to_user could not copy %lu bytes to user", ctu_return);

    retval = buf_size;
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
    char *kbuf = NULL;
    unsigned long cfu_return;
    struct aesd_dev *dev = (struct aesd_dev *)(filp->private_data);

	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
    
    /* Allocate requested memory in kernel buffer first */
    kbuf = kmalloc(count, GFP_KERNEL);
    if(kbuf == NULL)
        return retval;

    /* Copy the user-space buffer into the kbuf */
    cfu_return = copy_from_user(kbuf, buf, count);
    if(cfu_return != 0)
        PDEBUG("copy_from_user could not copy %lu bytes from user", cfu_return);

    /* Copy the kbuf ptr and size to the buffptr in the circular buffer */
    dev->aesd_actual_buffer.buffptr = kbuf;
    dev->aesd_actual_buffer.size   = count;

    /* Add it to the circular buffer */
    aesd_circular_buffer_add_entry(&(dev->aesd_circ_buffer),
                                   &(dev->aesd_actual_buffer)); 

    retval = count;
	return retval;
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));
    //PDEBUG("globalptr = %p\n", (void *)&aesd_device);
	/**
	 * TODO: initialize the AESD specific portion of the device
	 */
    aesd_circular_buffer_init(&(aesd_device.aesd_circ_buffer));
	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);
    //int index = 0;

	cdev_del(&aesd_device.cdev);
	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
     //AESD_CIRCULAR_BUFFER_FOREACH(&(aesd_device.aesd_actual_buffer), &(aesd_device.aesd_circ_buffer), index)
       // kfree(aesd_device.aesd_actual_buffer->buffptr);


	unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
