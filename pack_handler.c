#include <linux/init.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/sock.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>

MODULE_LICENSE("GPL");

#define MAX_IF_NUM 5
#define MAX_PROTO_NUM 10

struct stats_pck {
	int id_prot[MAX_PROTO_NUM];
	int counter[MAX_PROTO_NUM]; 
	int state;
	int proto_count;

	struct ip_lr_stats {
		int id_prot_ip[MAX_PROTO_NUM];
		int counter_ip[MAX_PROTO_NUM]; 
		int state_ip;
		int proto_count_ip;
	} ip_stats;
};

struct stats_pck rx_stats;

rx_handler_result_t (*old_func_ptr[MAX_IF_NUM])(struct sk_buff **);

rx_handler_result_t pack_handler (struct sk_buff **pskb) {

	struct sk_buff *skbuff = *pskb;
	struct iphdr * parser_ip_lr;
//	struct tcphdr * parser_tcp_hdr;
	int i = 0;
	int buff_size = 0;

	printk (KERN_INFO "\n");
	printk (KERN_INFO "proto_id = %#hx \n", ntohs(skbuff->protocol));

	buff_size = skbuff->transport_header - skbuff->network_header;
	if (buff_size > 0) {
		int j = 0;
		void * header_buff;
		header_buff = kmalloc (buff_size, GFP_KERNEL);
		memcpy (header_buff, skbuff->head + skbuff->network_header, buff_size);
		parser_ip_lr = (struct iphdr *) header_buff;
		printk (KERN_INFO "ip_header_proto = %hhu \n", parser_ip_lr->protocol);
		printk (KERN_INFO "ihl = %hhu \n", parser_ip_lr->ihl);
		printk (KERN_INFO "version = %hhu \n", parser_ip_lr->version);
		printk (KERN_INFO "tos = %hhu \n", parser_ip_lr->tos);
		printk (KERN_INFO "tot_len = %hu \n", parser_ip_lr->tot_len);
		printk (KERN_INFO "id = %hu \n", parser_ip_lr->id);
		printk (KERN_INFO "frag_off = %hu \n", parser_ip_lr->frag_off);
		printk (KERN_INFO "ttl = %hhu \n", parser_ip_lr->ttl);

		for (j = 0; j < rx_stats.ip_stats.proto_count_ip; j++) {
			if (parser_ip_lr->protocol == rx_stats.ip_stats.id_prot_ip[j]) {
				rx_stats.ip_stats.counter_ip[j]++;
				rx_stats.ip_stats.state_ip = 1;
				break;
			} else {
				rx_stats.ip_stats.state_ip = 0;
			}
		}

		if (0 == rx_stats.ip_stats.state_ip) {
			rx_stats.ip_stats.id_prot_ip[rx_stats.ip_stats.proto_count_ip] = parser_ip_lr->protocol;
			rx_stats.ip_stats.counter_ip[rx_stats.ip_stats.proto_count_ip]++;
			rx_stats.ip_stats.proto_count_ip++;
		}

		kfree(header_buff);
	}
	
	printk (KERN_INFO "\n");

	if (0 == buff_size) { 
		for (i = 0; i < rx_stats.proto_count; i++) {
			if (ntohs(skbuff->protocol) == rx_stats.id_prot[i]) {
				rx_stats.counter[i]++;
				rx_stats.state = 1;
				break;
			} else {
				rx_stats.state = 0;
			}
		}

		if (0 == rx_stats.state) {
			rx_stats.id_prot[rx_stats.proto_count] = ntohs(skbuff->protocol);
			rx_stats.counter[rx_stats.proto_count]++;
			rx_stats.proto_count++;
		}
	}

	return RX_HANDLER_PASS; 
}

static int __init mod_init (void) {

	struct net_device * dev;
	int i = 0; 

	dev = first_net_device (&init_net);
	
	while (dev) {
		rcu_assign_pointer(old_func_ptr[i], dev->rx_handler);
		rcu_assign_pointer(dev->rx_handler, pack_handler);
		i++;

		printk (KERN_INFO "name = %s \n", dev->name);
		printk (KERN_INFO "RX packets = %lu \n", dev->stats.rx_packets);
		printk (KERN_INFO "TX packets = %lu \n", dev->stats.tx_packets);
		dev = next_net_device(dev);
	}

	return 0;
}

static void  __exit mod_exit (void) {

	struct type_table {
		int type;
		char * name;
	};

	struct type_table proto_table [] = {
		{0x800, "IPv4"},
		{0x86DD, "IPv6"},
		{0x806, "ARP"},
	};


	struct type_table proto_table_ip [] = {
		{0x1, "ICMP"},
		{0x2, "IGMP"},
		{0x6, "TCP"},
		{0x11, "UDP"},
	};

	struct net_device * dev;
	int i = 0;
	int j = 0;
	dev = first_net_device(&init_net);
	while (dev) {
		rcu_assign_pointer(dev->rx_handler, old_func_ptr[i]);
		dev = next_net_device(dev);	
	}

	for (i = 0; i < rx_stats.proto_count; i++) {
		j = 0;
		rx_stats.state = 0;
		for (j = 0; j < sizeof(proto_table)/sizeof(struct type_table); j++) {
			if (rx_stats.id_prot[i] == proto_table[j].type) {
				printk (KERN_INFO "Protocol: %s\n", proto_table[j].name);
				rx_stats.state = 1;
				break;
			}
		}

		if (0 == rx_stats.state) {
			printk (KERN_INFO "Protocol: %#hx (other) \n", rx_stats.id_prot[i]);
		}

		printk (KERN_INFO "Number: %d \n", rx_stats.counter[i]);
	}

	for (j = 0; j < rx_stats.ip_stats.proto_count_ip; j++) {
		i = 0;
		rx_stats.ip_stats.state_ip = 0;
		for (i = 0; i < sizeof(proto_table_ip)/sizeof(struct type_table); i++) {
			if (rx_stats.ip_stats.id_prot_ip[j] == proto_table_ip[i].type) {
				printk (KERN_INFO "Protocol: %s\n", proto_table_ip[i].name);
				rx_stats.ip_stats.state_ip = 1;
				break;
			}
		}

		if (0 == rx_stats.ip_stats.state_ip) {
			printk (KERN_INFO "Protocol: %#hx\n", rx_stats.ip_stats.id_prot_ip[j]);
		}
		printk (KERN_INFO "Number: %d \n", rx_stats.ip_stats.counter_ip[j]);
	}
}

module_init(mod_init);
module_exit(mod_exit);
