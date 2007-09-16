//#include <linux/config.h>
#include <linux/types.h>
//#include <linux/miscdevice.h>
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
//#include <linux/cdev.h>

//#include <linux/soundcard.h>
//#include <asm/param.h>
//#include <asm/dma.h>
//#include <asm/irq.h>

/*****************************************************************************/
/*
#define LEON_CLOCK (54000000)
#define	AC97_MAJOR 120
#define AC97_IRQ 13
#define AC97_HW_BUFFER 4096  //in samples
#define	MAX_AC97_SW_BUFFER 65536 //in bytes
#define DEFAULT_SAMPLE_RATE 44100
*/
#define	THEORA_MAJOR 121
#define THEORA_IRQ 13
#define DEVICE_FILE_NAME "theora"
#define	MAX_THEORA_SW_BUFFER 65536 //in bytes
/*****************************************************************************/

/*
 *	buffers (right and left) for storing audio samples.
 */
static int theora_buf[MAX_THEORA_SW_BUFFER];


struct theora_regs_t {
  volatile int a;
  volatile int b;
  volatile int c;
};

struct _data
{
    int			leu;	// Função 1=saída, 2=entrada
    int			escreveu;	// Porta de entrada ou saída
    int		data;	// Dado de entrada ou saída
};

  volatile int * a;
  volatile int * b;
  volatile int * c;
  

//struct theora_regs_t  * theora_regs = (struct theora_regs_t *) 0x80000800;
int cs = 500, cr = 0, i = 0;
//struct theora_regs_t  * theora_regs = (struct theora_regs_t *) 0x80000800;
//struct wait_queue * ac97_waitchan = NULL;

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

static ssize_t theora_ioctl(struct inode *inode, struct file *filp, unsigned int nFunc,  unsigned long nParam) //unsigned long
{


	//b = ioremap(0x80000804, 4);
	//c = ioremap(0x80000808, 4);
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
    //while(*a == 0);
    dt->leu = 0;
    //if (i < 2100)
    if (*a == 1) {
      *a = dt->data;
      dt->leu = 1;
      if (cs == 500) {
        printk("[%d] Sending %d\n", i, dt->data);
        cs = 0;
      }
      i++;
      cs++;      
    }
    break;

  case 1:
    //while(*b == 0);
    dt->escreveu = 0;
   // if (i < 2100)
   //= *b;
    //while(*b == 0);
    if (*b == 1) {
      dt->escreveu = 1;
      dt->data = *c;
      printk("Receiving[%d] ",cr++);
      printk("= %d\n", dt->data);
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
    .read   = theora_read,
    .write   = theora_write,
    .ioctl   = theora_ioctl,
    .release   = theora_release,
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
	/* Register ac97 as character device */
	/*
	ret = misc_register(&theora_dev);
	if (ret) {
		printk(KERN_WARNING "Unable to register misc device.\n");
		return ret;
	}
*/
	result = register_chrdev(THEORA_MAJOR, DEVICE_FILE_NAME, &theora_fops);
	if (result < 0) {
		printk(KERN_WARNING "THEORA: can't get major %d\n", THEORA_MAJOR);
		return;
	}
a = ioremap(0x80000800, 4);
b = ioremap(0x80000804, 4);
c = ioremap(0x80000808, 4);

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

