#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>

MODULE_LICENSE("GPL");

#define MAX_IF_NUM 5

rx_handler_result_t (*old_func_ptr[MAX_IF_NUM])(struct sk_buff **);


rx_handler_result_t pack_handler (struct sk_buff **pskb) {
	
	struct sk_buff *skbuff = *pskb;
	
	printk (KERN_INFO "SK_BUFF_INFO: mac_len = %d", skbuff->mac_len);
	printk (KERN_INFO "proto_id = %#hx \n", ntohs(skbuff->protocol));

	return RX_HANDLER_PASS; 
}

static int __init mod_init (void) {

	struct net_device * dev;
	int i = 0; 

	dev = first_net_device (&init_net);
	
	while (dev) {
		old_func_ptr[i] = dev->rx_handler;
		dev->rx_handler = pack_handler;
		printk (KERN_INFO "ptr = %p \n", old_func_ptr[i]);
		i++;

		printk (KERN_INFO "name = %s \n", dev->name);

		printk (KERN_INFO "pointer = %p \n", dev->rx_handler);


		printk (KERN_INFO "RX packets = %lu \n", dev->stats.rx_packets);
		printk (KERN_INFO "TX packets = %lu \n", dev->stats.tx_packets);
		dev = next_net_device(dev);
		
	}

	return 0;
}

static void  __exit mod_exit (void) {

	printk (KERN_INFO "Bye! \n");
	struct net_device * dev;
	int i = 0;
	dev = first_net_device(&init_net);
	while (dev) {
		dev->rx_handler = old_func_ptr[i];
		dev = next_net_device(dev);	
	}
}

module_init(mod_init);
module_exit(mod_exit);
