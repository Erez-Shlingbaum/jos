#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	void *pg = &nsipcbuf;

	int r = sys_page_unmap(0, pg);
	if (r < 0)
		panic("ns_input: unmap failed %e\n", r);

	while (1)
	{
		sys_page_alloc(0, pg, PTE_P | PTE_U | PTE_W);

		struct jif_pkt *pkt = (struct jif_pkt *) &nsipcbuf.pkt;

		uint32_t recv_packet_size;
		while (1)
		{
			r = sys_try_recv_packet((uint8_t *) pkt->jp_data, PGSIZE - sizeof(uint32_t), &recv_packet_size);
			if (r == 0)
				break;
			else if (r == -E_INVAL)
				panic("ns_input: INVALID parameters");
			else if (r == -E_NET_QUEUE_EMPTY)
				sys_yield();
			else
				panic("ns_input: unknown error %e\n", r);
		}
//		cprintf("ns_input: received packet of length: %x\n", recv_packet_size);
//		for (int i = 0; i < 15; ++i)
//			cprintf("%x ", pkt->jp_data[i]);

		// Save received packet length
		pkt->jp_len = recv_packet_size;
		ipc_send(ns_envid, NSREQ_INPUT, pg, PTE_P | PTE_U | PTE_W);
		sys_page_unmap(0, pg);
	}
}
