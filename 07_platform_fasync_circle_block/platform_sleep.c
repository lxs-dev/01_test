#include <linux/module.h>
#include <linux/irqreturn.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/sched.h> 
#include <linux/poll.h>



#define DATA_TYPE  int
#define BUF_MAX   128
#define NEXT_POS(x) (x+1)%BUF_MAX

struct fasync_struct *fasync_key;


DATA_TYPE buf[BUF_MAX];
DATA_TYPE pread, pwrite;

int buf_is_empty(void)
{
	return (pread == pwrite);
}
int buf_is_full(void)
{
	return (pread == NEXT_POS(pwrite));
}

void put_data(DATA_TYPE data)
{
	if(buf_is_full()!=1)
	{
		buf[pwrite] = data;
		pwrite = NEXT_POS(pwrite);
	}else
	{
		printk("缓冲区已满\n");
	}
	
	return ;
}
DATA_TYPE get_data(void) //获取数据 -1：无数据
{
	DATA_TYPE data;
	if(buf_is_empty() != 1)
	{
		data = buf[pread];		
		pread = NEXT_POS(pread);
		return data;
	}else
	{
		printk("缓冲区已空\n");
	}
	return -1;
}

char *key_name = "lxs_key";
unsigned int key_major = 0;
struct class *key_class;
char data;
static DECLARE_WAIT_QUEUE_HEAD(gpio_key_wait);
static int g_key = 0; 


struct key_irq
{
	int gpio_num;
	struct gpio_desc *key_gpiod;
	unsigned int irq_num;
	enum of_gpio_flags flag;
};

struct key_irq *lxs_key_aittu;

static irqreturn_t key_hardler(int irq, void *dev_instance)
{
	int val = 0;
	struct key_irq *lxs_key_person = dev_instance;
	val = gpiod_get_value(lxs_key_person->key_gpiod);
	//printk("%s line %d\n", __FUNCTION__, __LINE__);
	wake_up_interruptible(&gpio_key_wait);
	put_data(val);
	kill_fasync(&fasync_key, SIGIO,POLL_IN);
	//g_key = 1;
	printk("gpio: %d irq : %d  val: %d\n" , lxs_key_person->gpio_num,lxs_key_person->irq_num,val);
	return IRQ_HANDLED;
}

static ssize_t key_read (struct file *file, char __user *buf , size_t size, loff_t *loff)
{
	int err;
	int data;
	//printk("%s line %d\n", __FUNCTION__, __LINE__);
	if((file->f_flags & O_NONBLOCK) && buf_is_empty() )
		return -EAGAIN;
	wait_event_interruptible(gpio_key_wait, !buf_is_empty());

	data = get_data();
	err = copy_to_user(buf, &data, 4);
	return 4;
}

static unsigned int key_poll(struct file *file, poll_table *wait)
{

	poll_wait(file, &gpio_key_wait, wait);
	return buf_is_empty() ? 0 : (POLLIN | POLLRDNORM);
}
static int key_fasync(int fd, struct file *file, int on)
{
	fasync_helper(fd, file, on, &fasync_key);
	return 0;
}


static const struct file_operations key_fops = {
	.owner		= THIS_MODULE,
	.read		= key_read,
	.poll       = key_poll,
	.fasync		= key_fasync
};

static int lxs_key_probe(struct platform_device *pdev)
{
	enum of_gpio_flags flags;
	struct device_node	*of_node = pdev->dev.of_node;
	int err;
	printk("%s line %d\n", __FUNCTION__, __LINE__);
	lxs_key_aittu =  kzalloc(sizeof(struct key_irq ),GFP_KERNEL);
	lxs_key_aittu->gpio_num = 	of_get_gpio_flags(of_node , 0 , &flags ); 
	lxs_key_aittu->irq_num = gpio_to_irq(lxs_key_aittu -> gpio_num);
	err = request_irq(lxs_key_aittu->irq_num ,key_hardler,IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,"lxs_key",lxs_key_aittu);
	lxs_key_aittu->key_gpiod =  gpio_to_desc(lxs_key_aittu->gpio_num);
	key_major  = register_chrdev(0,key_name,&key_fops);

	key_class  = class_create(THIS_MODULE, key_name);

	device_create(key_class, NULL, MKDEV(key_major, 0), NULL, key_name);
	return  0;	
}
static int lxs_key_remove(struct platform_device *pdev)
{
	printk("%s line %d\n", __FUNCTION__, __LINE__);

	free_irq(lxs_key_aittu->irq_num, lxs_key_aittu);

	device_destroy(key_class,MKDEV(key_major, 0));
	class_destroy(key_class);
	unregister_chrdev(key_major,key_name);
	kfree(lxs_key_aittu);
	return 0;
}
static const struct of_device_id lxs_key_table[] = {
	{.compatible = "lxs_key"},
	{ },
};
struct platform_driver key_plat = {
	.probe = lxs_key_probe,
	.remove = lxs_key_remove,
	.driver = {
		.name = "lxs_key",
		.of_match_table = lxs_key_table,
	},
};
static int __init lxs_key_init(void)
{
	int err;
	printk("%s line %d\n", __FUNCTION__, __LINE__);
	err = platform_driver_register(&key_plat);
	return 0;	
}
static void __exit lxs_key_exit(void)
{
	printk("%s line %d\n", __FUNCTION__, __LINE__);
	platform_driver_unregister(&key_plat);
}

module_init(lxs_key_init);
module_exit(lxs_key_exit);
MODULE_LICENSE("GPL");

