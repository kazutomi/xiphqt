/*
Theora Driver to Theora Hardware
Tested on LEON3 and linux-2.6.21.1
André Costa - andre.lnc@gmail.com
*/


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>

/*****************************************************************************/

#define	THEORA_MAJOR 121
#define THEORA_IRQ 13
#define DEVICE_FILE_NAME "theora"
/*****************************************************************************/


struct theora_regs_t {
  volatile int flag_send_data;   // Can the driver send a data to Theora Hardware?
  volatile int data_transmitted; // Data Transmitted  to Theora Hardware
  volatile int flag_read_data;   // Can the driver receive a data from Theora Hardware?
  volatile int data_received;    // Data received from Theora Hardware
};

struct _data
{
    int read;	// Driver read on Theora Hardware
    int wrote;	// Driver wrote on Theora Hardware
    int data;	// Read or transmitted data
};

struct theora_regs_t  * theora_regs;
int cs = 500, cr = 0, i = 0;

/*****************************************************************************/


static ssize_t theora_open(struct inode* pInode, struct file* pFile)
{
  printk(KERN_EMERG "Theora open\n");
  return(0);
}

static ssize_t theora_release(struct inode* pInode, struct file* pFile)
{
    printk(KERN_ALERT "device_release function\n");
    return 0;
}

// It is the main function. It is called from dct_decode.c in order to send and transmitt datas to Theora Hardware
static ssize_t theora_ioctl(struct inode *inode, struct file *filp, unsigned int nFunc,  unsigned long nParam) //unsigned long
{

	struct _data*  dt = kmalloc(sizeof(struct _data), GFP_KERNEL);
    if(!dt)
    {
        printk(KERN_ALERT "Can't allocate memory to dt\n");
        return -ENOMEM;
    }

    if(copy_from_user(dt, (void*)nParam, sizeof(struct _data)))
    {
        printk(KERN_ALERT "copy_from_user failed\n");
        kfree(dt);
        return -EFAULT;
    }
    
  switch (nFunc) {
  case 0:
    dt->read = 0;
    //if (i < 2100)
    if (theora_regs->flag_send_data == 1) { // Can I send a data to Theora Hardware?
      theora_regs->data_transmitted = dt->data;
      dt->read = 1;
      /*
      if (cs == 500) {
       // printk("[%d] Sending %d\n", i, dt->data);
        cs = 0;
      }
      i++;
      cs++;      */
    }
    break;

  case 1:
    //while(*b == 0);
    dt->wrote = 0;
   // if (i < 2100)
    //while(*b == 0);
    if (theora_regs->flag_read_data == 1) { // Can I receive a data from Theora Hardware?
      dt->wrote = 1;
      dt->data = theora_regs->data_received;
    //  printk("Receiving %d\n ",dt->data);
     // printk("= %d\n", dt->data);
      //printk("Receiving[%d] ",cr++);
   }
    break;
  }

   if(copy_to_user((void*)nParam, dt, sizeof(struct _data)))
    {
        printk(KERN_ALERT "copy_to_user failed\n");
        kfree(dt);
        return -EFAULT;
    }
    
   kfree(dt);
   
   return 0;
}

/*****************************************************************************/
static ssize_t theora_write(struct file* pFile,const char __user* pBuf, size_t len, loff_t* offset)
{
   printk(KERN_ALERT "device write\n");
   return 0;
}
/*****************************************************************************/
static ssize_t theora_read(struct file* pFile, char __user* pBuf, size_t len, loff_t* offset)
{
   return 0;
}

/*****************************************************************************/

/*
 *	Exported file operations structure for driver...
 */
struct file_operations theora_fops=
{
    .read    = theora_read,
    .write   = theora_write,
    .ioctl   = theora_ioctl,
    .release = theora_release,
    .open    = theora_open,
};
/*
static struct miscdevice theora_dev = {
	MISC_DYNAMIC_MINOR,
	"theora",
	&theora_fops
};
*/

static int theora_init(void)
{
  int result, ret;
    printk(KERN_INFO "Loading theora ...\n");
	/* Register Theora as character device */

	result = register_chrdev(THEORA_MAJOR, DEVICE_FILE_NAME, &theora_fops);
	if (result < 0) {
		printk(KERN_WARNING "THEORA: can't get major %d\n", THEORA_MAJOR);
		return;
	}
	
    theora_regs = ioremap(0x80000800, 16);

	printk ("LEON THEORA driver by Andre Costa (2007) - andre.lnc@gmail.com\n");
/*
    if(request_region(0x80000800, 16, "theora")==0)
    {
        printk(KERN_EMERG "theora, Can't request port region!\n");
        printk(KERN_EMERG " Check if port is not in use with cat /proc/ioports\n");
        unregister_chrdev(THEORA_MAJOR, DEVICE_FILE_NAME);
        return;

    }
*/

}

/*****************************************************************************/

static int __init exit(void)
{
    printk(KERN_INFO "Unloading theora ... \n");
  //  release_region(0x80000800, 16);
    unregister_chrdev(THEORA_MAJOR, DEVICE_FILE_NAME);
}

module_exit(exit);
module_init(theora_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andre");
MODULE_DESCRIPTION("Theora char driver");

