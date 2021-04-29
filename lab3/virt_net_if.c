#include <linux/module.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/in.h>
#include <net/arp.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/kernel.h>

static char* link = "enp0s3";
module_param(link, charp, 0);

static char* ifname = "vni%d";

static int port_dst = 0;
static int control_port = 15;

static unsigned char data[1500];
static struct net_device_stats stats;
static struct proc_dir_entry* entry;

struct net_history {
    int len;
    int *port_src;
    int *port_dst;     
};
struct net_history *udp_history = NULL;

static struct net_device *child = NULL;
struct priv {
	struct net_device *parent;
};

static char check_frame(struct sk_buff *skb, unsigned char data_shift) {
	struct iphdr *ip = (struct iphdr *)skb_network_header(skb);
	struct udphdr *udp = NULL;
	
	if (IPPROTO_UDP == ip->protocol) {
		udp = (struct udphdr*)((unsigned char*)ip + (ip->ihl * 4));

		if(ntohs(udp->dest) == control_port) {
			int data_len = 0;
			unsigned char *control_data_ptr = NULL;
			data_len = ntohs(udp->len) - sizeof(struct udphdr);
			control_data_ptr = (unsigned char*) (skb->data + sizeof(struct iphdr) + sizeof(struct udphdr)) + data_shift;
			memcpy(data, control_data_ptr, data_len);
			data[data_len] = '\0';
			int i = 0;
			int new_port = 0;

			while (i < data_len) {
				if ((int) data[i] >= '0' && (int) data[i] <= '9') {
					new_port = new_port*10+((int) data[i] - '0');
					i++;
				} else {
					break;
				}
			}
			if (new_port == 0) {
				printk(KERN_INFO "Message to control port should start with number\n");
			} else {
				port_dst = new_port;
				printk(KERN_INFO "Data resive port change to %d\n", new_port);
			}	
			return 3;

		} else if (ntohs(udp->dest) == port_dst) {
			if(udp_history == NULL){
				udp_history = kmalloc(sizeof(struct net_history), GFP_KERNEL);
				if (udp_history == NULL) {
					printk(KERN_ERR "Error: not enough memory.\n");
					return 0;
			    	} else {
					udp_history->len = 0;
					udp_history->port_src = NULL;
					udp_history->port_dst = NULL;
				}
	    		}

			int *new_buf_src = NULL;
			int *new_buf_dst = NULL;
			new_buf_src = krealloc(udp_history->port_src, (udp_history->len+1)*sizeof(int), GFP_KERNEL);
			new_buf_dst = krealloc(udp_history->port_dst, (udp_history->len+1)*sizeof(int), GFP_KERNEL);
			if (new_buf_src == NULL || new_buf_dst == NULL) {
				printk(KERN_ERR "Error: not enough memory.\n");
				return 0;
			}
	    		udp_history->len += 1;
	    		udp_history->port_src = new_buf_src;
	    		udp_history->port_dst = new_buf_dst;
			
	    		udp_history->port_src[udp_history->len-1] = ntohs(udp->source);
	    		udp_history->port_dst[udp_history->len-1] = ntohs(udp->dest);

			printk(KERN_INFO "Captured UDP datagram, IPsrc: %d.%d.%d.%d PORTsrc: %d\n",
					ntohl(ip->saddr) >> 24, (ntohl(ip->saddr) >> 16) & 0x00FF,
					(ntohl(ip->saddr) >> 8) & 0x0000FF, (ntohl(ip->saddr)) & 0x000000FF, ntohs(udp->source));
			
			printk(KERN_INFO "IPdst: %d.%d.%d.%d PORTdst: %d\n",
					ntohl(ip->daddr) >> 24, (ntohl(ip->daddr) >> 16) & 0x00FF,
					(ntohl(ip->daddr) >> 8) & 0x0000FF, (ntohl(ip->daddr)) & 0x000000FF, ntohs(udp->dest));
			return 2;
		}
		return 1;
	}
	return 0;
}

static rx_handler_result_t handle_frame(struct sk_buff **pskb) {
	if (check_frame(*pskb, 0)) {
	    	stats.rx_packets++;
	    	stats.rx_bytes += (*pskb)->len;
	}
return RX_HANDLER_PASS; 
} 

static int open(struct net_device *dev) {
	netif_start_queue(dev);
	printk(KERN_INFO "%s: device opened\n", dev->name);
	return 0; 
} 

static int stop(struct net_device *dev) {
	netif_stop_queue(dev);
	printk(KERN_INFO "%s: device closed\n", dev->name);
	return 0; 
} 

static netdev_tx_t start_xmit(struct sk_buff *skb, struct net_device *dev) {
	struct priv *priv = netdev_priv(dev);

	if (check_frame(skb, 14)) {
		stats.tx_packets++;
		stats.tx_bytes += skb->len;
	}

	if (priv->parent) {
		skb->dev = priv->parent;
		skb->priority = 1;
		dev_queue_xmit(skb);
		return 0;
	}
	return NETDEV_TX_OK;
}

static struct net_device_stats *get_stats(struct net_device *dev) {
	return &stats;
} 

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

static ssize_t proc_read(struct file *file, char __user * ubuf, size_t count, loff_t* ppos) 
{
    char *title = "-----------\nUDP ports\n-----------\nsrc\tdst\n";
    int title_offset = 42;
	size_t len = udp_history->len*24 + title_offset;
      	int i;
	char out_buf[12];
	if (*ppos > 0 || count < len)
      	{
	    	return 0;
      	}

    copy_to_user(ubuf, title, title_offset);    
    
	for (i=0; i<udp_history->len; i++)
	{
		int_to_str(udp_history->port_src[i], out_buf, '\t');
		if (copy_to_user(ubuf+title_offset+(12*2*i), out_buf, 12) != 0)
		{
			return -EFAULT;
		}
		int_to_str(udp_history->port_dst[i], out_buf, '\n');
		if (copy_to_user(ubuf+title_offset+(12*2*i+12), out_buf, 12) != 0)
		{
			return -EFAULT;
		}
	}
      	*ppos = len;
	printk(KERN_INFO "proc file: read()\n");
      	return len;
}


static ssize_t proc_write(struct file *file, const char __user * ubuf, size_t count, loff_t* ppos) 
{
  printk(KERN_DEBUG "Attempt to write proc file\n");
  return -1;
}

static struct net_device_ops crypto_net_device_ops = {
	.ndo_open = open,
	.ndo_stop = stop,
	.ndo_get_stats = get_stats,
	.ndo_start_xmit = start_xmit
};

static struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = proc_read,
  .write = proc_write,
};

static void setup(struct net_device *dev) {
	int i;
	ether_setup(dev);
	memset(netdev_priv(dev), 0, sizeof(struct priv));
	dev->netdev_ops = &crypto_net_device_ops;

	//fill in the MAC address with a phoney
	for (i = 0; i < ETH_ALEN; i++)
		dev->dev_addr[i] = (char)i;
} 

int __init vni_init(void) {
    udp_history = kmalloc(sizeof(struct net_history), GFP_KERNEL);
    if (udp_history == NULL){
	    printk(KERN_ERR "Error: not enough memory.\n");
	    return -ENOMEM;
    } else {	   
	    udp_history->len = 0;
	    udp_history->port_src = NULL;
	    udp_history->port_dst = NULL;	
    }
    entry = proc_create("vni0", 0444, NULL, &fops);
    printk(KERN_INFO "%s: proc file is created\n", THIS_MODULE->name);

    int err = 0;
	struct priv *priv;
	child = alloc_netdev(sizeof(struct priv), ifname, NET_NAME_UNKNOWN, setup);
	if (child == NULL) {
		printk(KERN_ERR "%s: allocate error\n", THIS_MODULE->name);
		return -ENOMEM;
	}
	priv = netdev_priv(child);
	priv->parent = __dev_get_by_name(&init_net, link); //parent interface
	if (!priv->parent) {
		printk(KERN_ERR "%s: no such net: %s\n", THIS_MODULE->name, link);
		free_netdev(child);
		return -ENODEV;
	}
	if (priv->parent->type != ARPHRD_ETHER && priv->parent->type != ARPHRD_LOOPBACK) {
		printk(KERN_ERR "%s: illegal net type\n", THIS_MODULE->name); 
		free_netdev(child);
		return -EINVAL;
	}

	//copy IP, MAC and other information
	memcpy(child->dev_addr, priv->parent->dev_addr, ETH_ALEN);
	memcpy(child->broadcast, priv->parent->broadcast, ETH_ALEN);
	if ((err = dev_alloc_name(child, child->name))) {
		printk(KERN_ERR "%s: allocate name, error %i\n", THIS_MODULE->name, err);
		free_netdev(child);
		return -EIO;
	}

	register_netdev(child);
	rtnl_lock();
	netdev_rx_handler_register(priv->parent, &handle_frame, NULL);
	rtnl_unlock();
	printk(KERN_INFO "Module %s loaded\n", THIS_MODULE->name);
	printk(KERN_INFO "%s: create link %s\n", THIS_MODULE->name, child->name);
	printk(KERN_INFO "%s: registered rx handler for %s\n", THIS_MODULE->name, priv->parent->name); 

    return 0;
}

void __exit vni_exit(void) {
	struct priv *priv = netdev_priv(child);
	kfree(udp_history);

	if (priv->parent) {
		rtnl_lock();
		netdev_rx_handler_unregister(priv->parent);
		rtnl_unlock();
		printk(KERN_INFO "%s: unregister rx handler for %s\n", THIS_MODULE->name, priv->parent->name);
	}
	unregister_netdev(child);
	free_netdev(child);
	printk(KERN_INFO "Module %s unloaded\n", THIS_MODULE->name); 

    proc_remove(entry);
	printk(KERN_INFO "%s: proc file is deleted\n", THIS_MODULE->name);
} 

module_init(vni_init);
module_exit(vni_exit);

MODULE_AUTHOR("Pavel Timofeev");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Intersept and filter of traffic");
