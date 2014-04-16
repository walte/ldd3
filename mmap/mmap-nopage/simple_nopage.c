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

struct page *simple_vma_nopage(struct vm_area_struct *vma,
		unsigned long address, int *type)
{
	struct page *pageptr;
	unsigned long offset = (address - vma->vm_start); 
	if (offset>PAGE_SIZE*2)
	{
		printk("out of size\n");
		return NULL;
	}
	printk("in vma_nopage: offset=%u\n", offset);

	if(offset<PAGE_SIZE) // the first page
		pageptr=virt_to_page(myaddr);
	else	// the second page
		pageptr=virt_to_page(myaddr+PAGE_SIZE);

	get_page(pageptr);

	return pageptr;
}

static int *simple_vma_fault(struct vm_area_struct *vma,
		struct vm_fault *vmf)
{
	pgoff_t pgoff = vmf->pgoff; 
	struct page *pageptr;
	//unsigned long offset = (address - vma->vm_start); 
	unsigned long offset = (pgoff << PAGE_SHIFT) + (vma->vm_pgoff << PAGE_SHIFT);
	if (offset>PAGE_SIZE*2)
	{
		printk("out of size\n");
		return NULL;
	}
	printk("in vma_nopage: offset=%u\n", offset);

	if(offset<PAGE_SIZE) // the first page
		pageptr=virt_to_page(myaddr);
	else	// the second page
		pageptr=virt_to_page(myaddr+PAGE_SIZE);

	get_page(pageptr);

	vmf->page = pageptr;

	return 0;
}

static struct vm_operations_struct simple_nopage_vm_ops = {
	.open =   simple_vma_open,
		.close =  simple_vma_close,
		.fault = simple_vma_fault,
		//.nopage = simple_vma_nopage,
};


static int simple_open (struct inode *inode, struct file *filp)
{
	return 0;
}

static int simple_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int simple_nopage_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	printk("enter simple_nopage_mmap: offset=%u, vma->vm_pgoff=%u\n", offset, vma->vm_pgoff);
	if (offset >= __pa(high_memory) || (filp->f_flags & O_SYNC))
		vma->vm_flags |= VM_IO;
	vma->vm_flags |= VM_RESERVED;

	vma->vm_ops = &simple_nopage_vm_ops;
	simple_vma_open(vma);
	return 0;
}

/*
 * Set up the cdev structure for a device.
 */
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

static struct file_operations simple_nopage_ops = {
	.owner   = THIS_MODULE,
	.open    = simple_open,
	.release = simple_release,
	.mmap    = simple_nopage_mmap,
};

/*
 * We export two simple devices.  There's no need for us to maintain any
 * special housekeeping info, so we just deal with raw cdevs.
 */
static struct cdev SimpleDevs;

/*
 * Module housekeeping.
 */
static int simple_init(void)
{
	int result;
	//unsigned int addr1, addr2;
	dev_t dev = MKDEV(simple_major, 0);

	/* Figure out our device number. */
	if (simple_major)
		result = register_chrdev_region(dev, 1, "simple_nopage");
	else {
		result = alloc_chrdev_region(&dev, 0, 1, "simple_nopage");
		simple_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "simple_nopage: unable to get major %d\n", simple_major);
		return result;
	}
	if (simple_major == 0)
		simple_major = result;

	/* Now set up two cdevs. */
	simple_setup_cdev(&SimpleDevs, 0, &simple_nopage_ops);

	myaddr = __get_free_pages(GFP_KERNEL, 1);
	if (!myaddr)
		return -ENOMEM;
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
