#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>

static dev_t first;
static struct cdev c_dev; 
static struct class *cl;
static struct proc_dir_entry* entry;
static int* history_buf = NULL;
static int history_len = 0;

static int my_open(struct inode *i, struct file *f);
static int my_close(struct inode *i, struct file *f);
static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t my_write(struct file *f, const char __user *buf,  size_t len, loff_t *off);
static ssize_t proc_write(struct file *file, const char __user * ubuf, size_t count, loff_t* ppos);
static ssize_t proc_read(struct file *file, char __user * ubuf, size_t count, loff_t* ppos);

static struct file_operations mychdev_fops =
{
  .owner = THIS_MODULE,
  .open = my_open,
  .release = my_close,
  .read = my_read,
  .write = my_write
};

static struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = proc_read,
  .write = proc_write,
};

static void int_to_str(int int_num, char* char_num, char end_char)
{
	char n;
	int len = 0;
        char_num[0] = 0;
        if(int_num < 0)
        {
                char_num[0] = '-';
                int_num = -int_num;
        }
        while(int_num != 0)
        {
                n = (int_num%10) + '0';
                char_num[10 - len] = n;
                int_num = int_num/10;
                len++;
        }
        while(len != ((char_num[0] == '-')?10:11))
        {
                char_num[10 - len] = 127;
                len++;
        }
        char_num[11] = end_char;
}

static int str_sum(char* iterator, size_t len){
	int sign = 0;
	int n = 0;
	int sum = 0;
	
	int i = 0;
    	for(i = 0; i < len; i++){
		if(iterator[i] == '-'){
	    		if(n != 0){
    				n = sign?-n:n;
    				sum = sum + n;
    				sign = 0;
    				n = 0;
	    		}
			sign = 1;
		}else if(iterator[i] == '+'){
		}else if(iterator[i] >= '0' && iterator[i] <= '9'){
			n = n*10 + (iterator[i] - '0');
		}else{
			n = sign?-n:n;
			sum = sum + n;
			sign = 0;
			n = 0;
		}
	}
    	return sum;
}

static int my_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: open()\n");
  return 0;
}

static int my_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: close()\n");
  return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	int i;
	size_t length = history_len*sizeof(int);    
    	if(*off > 0 || len < length)
	{
		return 0;
    	}
	printk(KERN_INFO "Driver: read()\n");
	for (i=0; i<history_len; i++)
        {
		printk(KERN_INFO "Sum%d: %d", i+1, history_buf[i]);
	}
	*off = length;
	return length;
}

static ssize_t my_write(struct file *f, const char __user *buf,  size_t len, loff_t *off)
{
	char* rename_cmd = "rename_log";
	char* input_buf = NULL;
	int* new_buf = NULL;
	int sum;
	input_buf = (char*) kmalloc(len*sizeof(char), GFP_KERNEL);
	if(input_buf == NULL) 
	{
		printk(KERN_ERR "Error: not enough memory.");
		return -ENOMEM;
    	}
	if(copy_from_user(input_buf, buf, len) !=0)
	{
		return -EFAULT;
	}
	
	if(strncmp(rename_cmd, input_buf, strlen(rename_cmd)) == 0)
	{
		char* file_name = (char*) kmalloc(len-strlen(rename_cmd)-1, GFP_KERNEL);
		sscanf(input_buf, "rename_log %s", file_name);

		proc_remove(entry);
		entry = proc_create(file_name, 0444, NULL, &fops);
		if(entry == NULL)
		{
			printk(KERN_ERR "/Device: failed to rename log file to \"%s\"", file_name);
			return -ENOMEM;
		}
		kfree(file_name);
		kfree(input_buf);
		kfree(history_buf);
		history_buf = NULL;
		history_len = 0;
		return len;
	}

   	sum = str_sum(input_buf, len);
	kfree(input_buf);
    	input_buf = NULL;
    	history_len = history_len + 1;
    	new_buf = (int*) krealloc(history_buf, history_len*sizeof(int), GFP_KERNEL);
	if(new_buf == NULL)
	{
		printk(KERN_ERR "Error: not enough memory.");
		return -ENOMEM;
    	}
    	history_buf = new_buf;
    	new_buf = NULL;    
    	history_buf[history_len-1] = sum;
	printk(KERN_INFO "Driver: write(%d)\n", sum);
    	return len;
}

static ssize_t proc_write(struct file *file, const char __user * ubuf, size_t count, loff_t* ppos) 
{
  printk(KERN_DEBUG "Attempt to write proc file");
  return -1;
}

static ssize_t proc_read(struct file *file, char __user * ubuf, size_t count, loff_t* ppos) 
{
	size_t len = history_len*12;
      	int i;
	char out_buf[12];
	if (*ppos > 0 || count < len)
      	{
	    	return 0;
      	}
	for (i=0; i<history_len; i++)
	{
		int_to_str(history_buf[i], out_buf, '\n');
		if (copy_to_user(ubuf+(12*i), out_buf, 12) != 0)
		{
			return -EFAULT;
		}
	}

      	*ppos = len;
	printk(KERN_INFO "proc file: read()\n");
      	return len;
}

static int __init ch_drv_init(void)
{
    printk(KERN_INFO "Hello!\n");
    if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0)
	  {
		return -1;
	  }
    if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
	  {
		unregister_chrdev_region(first, 1);
		return -1;
	  }
    if (device_create(cl, NULL, first, NULL, "var3") == NULL)
	  {
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	  }
    cdev_init(&c_dev, &mychdev_fops);
    if (cdev_add(&c_dev, first, 1) == -1)
	  {
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	  }
    
    entry = proc_create("var3", 0444, NULL, &fops);
    printk(KERN_INFO "%s: proc file is created\n", THIS_MODULE->name);
    return 0;
}
 
static void __exit ch_drv_exit(void)
{
    	kfree(history_buf);
	
    	cdev_del(&c_dev);
    	device_destroy(cl, first);
    	class_destroy(cl);
    	unregister_chrdev_region(first, 1);
	printk(KERN_INFO "Bye!!!\n");

    	proc_remove(entry);
    	printk(KERN_INFO "%s: proc file is deleted\n", THIS_MODULE->name);
}
 
module_init(ch_drv_init);
module_exit(ch_drv_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pavel Timofeev);
MODULE_DESCRIPTION("Char device driver");

