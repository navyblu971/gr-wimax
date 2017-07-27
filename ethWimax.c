#define MODULE             
	#define __KERNEL__	 
	


#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/interrupt.h> /* mark_bh */

#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>

#include "ofdm.h"

#include <linux/in6.h>
#include <asm/checksum.h>


extern void addDataToOFDM (unsigned char *  , int ) ;
	
/*
 * Transmitter lockup simulation, normally disabled.
 */
static int lockup = 0;
module_param(lockup, int, 0);

static int timeout = ofdm_TIMEOUT;
module_param(timeout, int, 0);

/*
 * Do we run in NAPI mode?
 */
static int use_napi = 0;
module_param(use_napi, int, 0);


/*
 * A structure representing an in-flight packet.
 */
struct ofdm_packet {
	struct ofdm_packet *next;
	struct net_device *dev;
	int	datalen;
	u8 data[ETH_DATA_LEN];
};

int pool_size = 8;
module_param(pool_size, int, 0);

/*
 * This structure is private to each device. It is used to pass
 * packets in and out, so there is place for a packet
 */

struct ofdm_priv {
	struct net_device_stats stats;
	int status;
	struct ofdm_packet *ppool;
	struct ofdm_packet *rx_queue;  /* List of incoming packets */
	int rx_int_enabled;
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	spinlock_t lock;
};

static void ofdm_tx_timeout(struct net_device *dev);
static void (*ofdm_interrupt)(int, void *, struct pt_regs *);

/*
 * Set up a device's packet pool.
 */
void ofdm_setup_pool(struct net_device *dev)
{
	struct ofdm_priv *priv = netdev_priv(dev);
	int i;
	struct ofdm_packet *pkt;

	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof (struct ofdm_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
			return;
		}
		pkt->dev = dev;
		pkt->next = priv->ppool;
		priv->ppool = pkt;
	}
}

void ofdm_teardown_pool(struct net_device *dev)
{
	struct ofdm_priv *priv = netdev_priv(dev);
	struct ofdm_packet *pkt;

	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		kfree (pkt);
		/* FIXME - in-flight packets ? */
	}
}

/*
 * Buffer/pool management.
 */
struct ofdm_packet *ofdm_get_tx_buffer(struct net_device *dev)
{
	struct ofdm_priv *priv = netdev_priv(dev);
	unsigned long flags;
	struct ofdm_packet *pkt;

	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->ppool;
	priv->ppool = pkt->next;
	if (priv->ppool == NULL) {
		printk (KERN_INFO "Pool empty\n");
		netif_stop_queue(dev);
	}
	spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}


void ofdm_release_buffer(struct ofdm_packet *pkt)
{
	unsigned long flags;
	struct ofdm_priv *priv = netdev_priv(pkt->dev);

	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->ppool;
	priv->ppool = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
	if (netif_queue_stopped(pkt->dev) && pkt->next == NULL)
		netif_wake_queue(pkt->dev);
}

void ofdm_enqueue_buf(struct net_device *dev, struct ofdm_packet *pkt)
{
	unsigned long flags;
	struct ofdm_priv *priv = netdev_priv(dev);

	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->rx_queue;  /* FIXME - misorders packets */
	priv->rx_queue = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
}

struct ofdm_packet *ofdm_dequeue_buf(struct net_device *dev)
{
	struct ofdm_priv *priv = netdev_priv(dev);
	struct ofdm_packet *pkt;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->rx_queue;
	if (pkt != NULL)
		priv->rx_queue = pkt->next;
	spin_unlock_irqrestore(&priv->lock, flags);
	return pkt;
}

/*
 * Enable and disable receive interrupts.
 */
static void ofdm_rx_ints(struct net_device *dev, int enable)
{
	struct ofdm_priv *priv = netdev_priv(dev);
	priv->rx_int_enabled = enable;
}


/*
 * Open and close
 */

int ofdm_open(struct net_device *dev)
{
	/* request_region(), request_irq(), ....  (like fops->open) */

	/*
	 * Assign the hardware address of the board: use "\0SNULx", where
	 * x is 0 or 1. The first byte is '\0' to avoid being a multicast
	 * address (the first byte of multicast addrs is odd).
	 */
	memcpy(dev->dev_addr, "\0SNUL0", ETH_ALEN);
	if (dev == ofdm_devs[1])
		dev->dev_addr[ETH_ALEN-1]++; /* \0SNUL1 */
	netif_start_queue(dev);
	return 0;
}

int ofdm_release(struct net_device *dev)
{
    /* release ports, irq and such -- like fops->close */

	netif_stop_queue(dev); /* can't transmit any more */
	return 0;
}

/*
 * Configuration changes (passed on by ifconfig)
 */
int ofdm_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) /* can't act on a running interface */
		return -EBUSY;

	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "ofdm: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}

	/* Allow changing the IRQ */
	if (map->irq != dev->irq) {
		dev->irq = map->irq;
        	/* request_irq() is delayed to open-time */
	}

	/* ignore other fields */
	return 0;
}

/*
 * Receive a packet: retrieve, encapsulate and pass over to upper levels
 */
void ofdm_rx(struct net_device *dev, struct ofdm_packet *pkt)
{
	struct sk_buff *skb;
	struct ofdm_priv *priv = netdev_priv(dev);

	/*
	 * The packet has been retrieved from the transmission
	 * medium. Build an skb around it, so upper layers can handle it
	 */
	skb = dev_alloc_skb(pkt->datalen + 2);
	if (!skb) {
		if (printk_ratelimit())
			printk(KERN_NOTICE "ofdm rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		goto out;
	}
	skb_reserve(skb, 2); /* align IP on 16B boundary */
	memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);

	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += pkt->datalen;
	netif_rx(skb);
  out:
	return;
}


/*
 * The poll implementation.
 */
static int ofdm_poll(struct net_device *dev, int *budget)
{
	int npackets = 0, quota = min(dev->quota, *budget);
	struct sk_buff *skb;
	struct ofdm_priv *priv = netdev_priv(dev);
	struct ofdm_packet *pkt;

	while (npackets < quota && priv->rx_queue) {
		pkt = ofdm_dequeue_buf(dev);
		skb = dev_alloc_skb(pkt->datalen + 2);
		if (! skb) {
			if (printk_ratelimit())
				printk(KERN_NOTICE "ofdm: packet dropped\n");
			priv->stats.rx_dropped++;
			ofdm_release_buffer(pkt);
			continue;
		}
		skb_reserve(skb, 2); /* align IP on 16B boundary */
		memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);
		skb->dev = dev;
		skb->protocol = eth_type_trans(skb, dev);
		skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
		netif_receive_skb(skb);

        	/* Maintain stats */
		npackets++;
		priv->stats.rx_packets++;
		priv->stats.rx_bytes += pkt->datalen;
		ofdm_release_buffer(pkt);
	}
	/* If we processed all packets, we're done; tell the kernel and reenable ints */
	*budget -= npackets;
	dev->quota -= npackets;
	if (! priv->rx_queue) {
		netif_rx_complete(dev);
		ofdm_rx_ints(dev, 1);
		return 0;
	}
	/* We couldn't process everything. */
	return 1;
}


/*
 * The typical interrupt entry point
 */
static void ofdm_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	struct ofdm_priv *priv;
	struct ofdm_packet *pkt = NULL;
	/*
	 * As usual, check the "device" pointer to be sure it is
	 * really interrupting.
	 * Then assign "struct device *dev"
	 */
	struct net_device *dev = (struct net_device *)dev_id;
	/* ... and check with hw if it's really ours */

	/* paranoid */
	if (!dev)
		return;

	/* Lock the device */
	priv = netdev_priv(dev);
	spin_lock(&priv->lock);

	/* retrieve statusword: real netdevices use I/O instructions */
	statusword = priv->status;
	priv->status = 0;
	if (statusword & ofdm_RX_INTR) {
		/* send it to ofdm_rx for handling */
		pkt = priv->rx_queue;
		if (pkt) {
			priv->rx_queue = pkt->next;
			ofdm_rx(dev, pkt);
		}
	}
	if (statusword & ofdm_TX_INTR) {
		/* a transmission is over: free the skb */
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += priv->tx_packetlen;
		dev_kfree_skb(priv->skb);
	}

	/* Unlock the device and we are done */
	spin_unlock(&priv->lock);
	if (pkt) ofdm_release_buffer(pkt); /* Do this outside the lock! */
	return;
}

/*
 * A NAPI interrupt handler.
 */
static void ofdm_napi_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	struct ofdm_priv *priv;

	/*
	 * As usual, check the "device" pointer for shared handlers.
	 * Then assign "struct device *dev"
	 */
	struct net_device *dev = (struct net_device *)dev_id;
	/* ... and check with hw if it's really ours */

	/* paranoid */
	if (!dev)
		return;

	/* Lock the device */
	priv = netdev_priv(dev);
	spin_lock(&priv->lock);

	/* retrieve statusword: real netdevices use I/O instructions */
	statusword = priv->status;
	priv->status = 0;
	if (statusword & ofdm_RX_INTR) {
		ofdm_rx_ints(dev, 0);  /* Disable further interrupts */
		netif_rx_schedule(dev);
	}
	if (statusword & ofdm_TX_INTR) {
        	/* a transmission is over: free the skb */
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += priv->tx_packetlen;
		dev_kfree_skb(priv->skb);
	}

	/* Unlock the device and we are done */
	spin_unlock(&priv->lock);
	return;
}



/*
 * Transmit a packet (low level interface)
 */
static void ofdm_hw_tx(char *buf, int len, struct net_device *dev)
{
	/*
	 * This function deals with hw details. This interface loops
	 * back the packet to the other ofdm interface (if any).
	 * In other words, this function implements the ofdm behaviour,
	 * while all other procedures are rather device-independent
	 */
	struct iphdr *ih;
	struct net_device *dest;
	struct ofdm_priv *priv;
	u32 *saddr, *daddr;
	struct ofdm_packet *tx_buffer;

	/* I am paranoid. Ain't I? */
	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		printk("ofdm: Hmm... packet too short (%i octets)\n",
				len);
		return;
	}

	if (0) { /* enable this conditional to look at the data */
		int i;
		PDEBUG("len is %i\n" KERN_DEBUG "data:",len);
		for (i=14 ; i<len; i++)
			printk(" %02x",buf[i]&0xff);
		printk("\n");
	}
	/*
	 * Ethhdr is 14 bytes, but the kernel arranges for iphdr
	 * to be aligned (i.e., ethhdr is unaligned)
	 */
	ih = (struct iphdr *)(buf+sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
	((u8 *)daddr)[2] ^= 1;

	ih->check = 0;         /* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);

	if (dev == ofdm_devs[0])
		PDEBUGG("%08x:%05i --> %08x:%05i\n",
				ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source),
				ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest));
	else
		PDEBUGG("%08x:%05i <-- %08x:%05i\n",
				ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest),
				ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source));

	/*
	 * Ok, now the packet is ready for transmission: first simulate a
	 * receive interrupt on the twin device, then  a
	 * transmission-done on the transmitting device
	 */
	dest = ofdm_devs[dev == ofdm_devs[0] ? 1 : 0];
	priv = netdev_priv(dest);
	tx_buffer = ofdm_get_tx_buffer(dev);
	tx_buffer->datalen = len;
	memcpy(tx_buffer->data, buf, len);
	ofdm_enqueue_buf(dest, tx_buffer);
	if (priv->rx_int_enabled) {
		priv->status |= ofdm_RX_INTR;
		ofdm_interrupt(0, dest, NULL);
	}

	priv = netdev_priv(dev);
	priv->tx_packetlen = len;
	priv->tx_packetdata = buf;
	priv->status |= ofdm_TX_INTR;
	if (lockup && ((priv->stats.tx_packets + 1) % lockup) == 0) {
        	/* Simulate a dropped transmit interrupt */
		netif_stop_queue(dev);
		PDEBUG("Simulate lockup at %ld, txp %ld\n", jiffies,
				(unsigned long) priv->stats.tx_packets);
	}
	else
		ofdm_interrupt(0, dev, NULL);
}

/*
 * Transmit a packet (called by the kernel)
 */
int ofdm_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	char *data, shortpkt[ETH_ZLEN];
	struct ofdm_priv *priv = netdev_priv(dev);
	
	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	dev->trans_start = jiffies; /* save the timestamp */

	/* Remember the skb, so we can free it at interrupt time */
	priv->skb = skb;

	/* actual deliver of data is device-specific, and not shown here */
	//ofdm_hw_tx(data, len, dev);
	addDataToOFDM(data, len) ;
	return 0; /* Our simple device can not fail */
}

/*
 * Deal with a transmit timeout.
 */
void ofdm_tx_timeout (struct net_device *dev)
{
	struct ofdm_priv *priv = netdev_priv(dev);

	PDEBUG("Transmit timeout at %ld, latency %ld\n", jiffies,
			jiffies - dev->trans_start);
        /* Simulate a transmission interrupt to get things moving */
	priv->status = ofdm_TX_INTR;
	ofdm_interrupt(0, dev, NULL);
	priv->stats.tx_errors++;
	netif_wake_queue(dev);
	return;
}



/*
 * Ioctl commands
 */
int ofdm_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	PDEBUG("ioctl\n");
	return 0;
}

/*
 * Return statistics to the caller
 */
struct net_device_stats *ofdm_stats(struct net_device *dev)
{
	struct ofdm_priv *priv = netdev_priv(dev);
	return &priv->stats;
}

/*
 * This function is called to fill up an eth header, since arp is not
 * available on the interface
 */
int ofdm_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *) skb->data;
	struct net_device *dev = skb->dev;

	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
	return 0;
}


int ofdm_header(struct sk_buff *skb, struct net_device *dev,
                unsigned short type, void *daddr, void *saddr,
                unsigned int len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest,   daddr ? daddr : dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
	return (dev->hard_header_len);
}





/*
 * The "change_mtu" method is usually not needed.
 * If you need it, it must be like this.
 */
int ofdm_change_mtu(struct net_device *dev, int new_mtu)
{
	unsigned long flags;
	struct ofdm_priv *priv = netdev_priv(dev);
	spinlock_t *lock = &priv->lock;

	/* check ranges */
	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;
	/*
	 * Do anything you need, and the accept the value
	 */
	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	return 0; /* success */
}

/*
 * The init function (sometimes called probe).
 * It is invoked by register_netdev()
 */
void ofdm_init(struct net_device *dev)
{
	struct ofdm_priv *priv;
#if 0
    	/*
	 * Make the usual checks: check_region(), probe irq, ...  -ENODEV
	 * should be returned if no device found.  No resource should be
	 * grabbed: this is done on open().
	 */
#endif

    	/*
	 * Then, assign other fields in dev, using ether_setup() and some
	 * hand assignments
	 */
	ether_setup(dev); /* assign some of the fields */

	dev->open            = ofdm_open;
	dev->stop            = ofdm_release;
	dev->set_config      = ofdm_config;
	dev->hard_start_xmit = ofdm_tx;
	dev->do_ioctl        = ofdm_ioctl;
	dev->get_stats       = ofdm_stats;
	dev->change_mtu      = ofdm_change_mtu;
	dev->rebuild_header  = ofdm_rebuild_header;
	dev->hard_header     = ofdm_header;
	dev->tx_timeout      = ofdm_tx_timeout;
	dev->watchdog_timeo = timeout;
	if (use_napi) {
		dev->poll        = ofdm_poll;
		dev->weight      = 2;
	}
	/* keep the default flags, just add NOARP */
	dev->flags           |= IFF_NOARP;
	dev->features        |= NETIF_F_NO_CSUM;
	dev->hard_header_cache = NULL;      /* Disable caching */

	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct ofdm_priv));
	spin_lock_init(&priv->lock);
	ofdm_rx_ints(dev, 1);		/* enable receive interrupts */
	ofdm_setup_pool(dev);
}

/*
 * The devices
 */

struct net_device *ofdm_devs[2];



/*
 * Finally, the module stuff
 */

void ofdm_cleanup(void)
{
	int i;

	for (i = 0; i < 2;  i++) {
		if (ofdm_devs[i]) {
			unregister_netdev(ofdm_devs[i]);
			ofdm_teardown_pool(ofdm_devs[i]);
			free_netdev(ofdm_devs[i]);
		}
	}
	return;
}




int ofdm_init_module(void)
{
	int result, i, ret = -ENOMEM;

	ofdm_interrupt = use_napi ? ofdm_napi_interrupt : ofdm_regular_interrupt;

	/* Allocate the devices */
	ofdm_devs[0] = alloc_netdev(sizeof(struct ofdm_priv), "sn%d",
			ofdm_init);
	ofdm_devs[1] = alloc_netdev(sizeof(struct ofdm_priv), "sn%d",
			ofdm_init);
	if (ofdm_devs[0] == NULL || ofdm_devs[1] == NULL)
		goto out;

	ret = -ENODEV;
	for (i = 0; i < 2;  i++)
		if ((result = register_netdev(ofdm_devs[i])))
			printk("ofdm: error %i registering device \"%s\"\n",
					result, ofdm_devs[i]->name);
		else
			ret = 0;
   out:
	if (ret)
		ofdm_cleanup();
	return ret;
}


module_init(ofdm_init_module);
module_exit(ofdm_cleanup);
