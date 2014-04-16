/*
 * Simple - REALLY simple memory mapping demonstration.
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: simple.c,v 1.12 2005/01/31 16:15:31 rubini Exp $
 */

//#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>   /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <asm/page.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gfp.h>

static unsigned char *myaddr=NULL;
static int simple_major = 0;
module_param(simple_major, int, 0);
MODULE_AUTHOR("Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");


/*
 * Common VMA ops.
 */

void simple_vma_open(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "Simple VMA open, virt %lx, phys %lx\n",
			vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

void simple_vma_close(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "Simple VMA close.\n");
}

static struct vm_operations_struct simple_remap_vm_ops = {
	.open =  simple_vma_open,
	.close = simple_vma_close,
};

static int simple_open (struct inode *inode, struct file *filp)
{
	return 0;
}

static int simple_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int simple_remap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_pgoff = __pa(myaddr)>>PAGE_SHIFT;

	printk("simple_remap_mmap\n");
	printk("start=%x; end=%x\n", vma->vm_start, vma->vm_end);
	printk("size=%u\n", vma->vm_end-vma->vm_start); 
	printk("page num=%x\n", vma->vm_pgoff);
	
	if ( remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
		vma->vm_end - vma->vm_start,
		vma->vm_page_prot) )
		return -EAGAIN;
	
	vma->vm_ops = &simple_remap_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	simple_vma_open(vma);
	
	return 0;	
}

static struct file_operations simple_remap_ops = {
	.owner   = THIS_MODULE,
	.open    = simple_open,
	.release = simple_release,
	.mmap    = simple_remap_mmap,
};


static void simple_setup_cdev(struct cdev *dev, int minor,
		struct file_operations *fops)
{
	int err, devno = MKDEV(simple_major, minor);

	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	err = cdev_add (dev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk (KERN_NOTICE "Error %d adding simple%d", err, minor);
}


/*
 * Our various sub-devices.
 */
/* Device 0 uses remap_pfn_range */
static struct cdev SimpleDevs;

/*
 * Module housekeeping.
 */
static int simple_init(void)
{
	int result;
	dev_t dev = MKDEV(simple_major, 0);

	if (simple_major)
		result = register_chrdev_region(dev, 1, "simple_remap");
	else {
		result = alloc_chrdev_region(&dev, 0, 1, "simple_remap");
		simple_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "simple_remap: unable to get major %d\n", simple_major);
		return result;
	}
	if (simple_major == 0)
		simple_major = result;

	/* Now set up two cdevs. */
	simple_setup_cdev(&SimpleDevs, 0, &simple_remap_ops);

	myaddr = __get_free_pages(GFP_KERNEL, 1);
	if (!myaddr)
		return -ENOMEM;

	SetPageReserved(virt_to_page(myaddr));
	SetPageReserved(virt_to_page(myaddr+PAGE_SIZE));
	// for test
	strcpy(myaddr, "1234567890");
	strcpy(myaddr+PAGE_SIZE, "abcdefghij");
	return 0;
}


static void simple_cleanup(void)
{
	cdev_del(&SimpleDevs);
	unregister_chrdev_region(MKDEV(simple_major, 0), 1);
}


module_init(simple_init);
module_exit(simple_cleanup);
