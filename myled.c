#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/io.h>

MODULE_AUTHOR("yehuh");
MODULE_DESCRIPTION("led driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32* gpio_base = NULL;

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos){
	char c_buff;
	if(copy_from_user(&c_buff, buf, sizeof(char))){
		return -EFAULT;
	}
	if(c_buff == '0'){
		gpio_base[10] = 1 << 25;//set 25 th bit in 0x28 (clear output)
	}else if(c_buff == '1'){
		gpio_base[7] = 1 << 25; //set 25 th bit in 0x1C (set out put)
	}

	return 1;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write
};

static int __init init_mod(void){
	int retval;
	u32 gpio_base_addr = 0x3F200000;
	u8 region_of_map = 0xA0;
	gpio_base = ioremap(gpio_base_addr, region_of_map);

	const u32 led = 25;		//gpio 25
	const u32 index = led/10;	//10 gpio for 4 bytes
	const u32 shift = (led % 10)*3;	//3 bits for 1 gpio
	const u32 mask = ~(0x07 << shift);
	gpio_base[index] = ((gpio_base[index] & mask) | /*clear gpio 25 related bits*/
		(0x01 << shift)); //set gpio 25 as output

	retval = alloc_chrdev_region(&dev, 0, 1, "myled");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev fail!!.\n");
		return retval;
	}
	cdev_init(&cdv, &led_fops);
	cdv.owner = THIS_MODULE;
	retval = cdev_add(&cdv, dev, 1);

	if(retval < 0){
		printk(KERN_ERR "acdev_add() fail. major:%d, minor:%d\n", MAJOR(dev), MINOR(dev));
		return retval;
	}
	printk(KERN_INFO "%s is loaded. major:%d.\n",__FILE__,MAJOR(dev));

	cls = class_create(THIS_MODULE,"myled");
	if(IS_ERR(cls)){
		printk(KERN_ERR "class_create() fail.");
		return PTR_ERR(cls);
	}
	device_create(cls, NULL, dev, NULL, "myled%d", MINOR(dev));
	return 0;
}

static void __exit cleanup_mod(void){
	cdev_del(&cdv);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s is unloaded. major:%d.\n",__FILE__,MAJOR(dev));
}

module_init(init_mod);
module_exit(cleanup_mod);
