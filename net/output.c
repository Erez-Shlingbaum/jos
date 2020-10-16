#include <kern/e1000.h>
#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

	void *pg = &nsipcbuf;

	while (1)
	{
		envid_t from_env;
		int perm_from;
//		cprintf("ns_output: before ipc recv\n");
		int r = ipc_recv(&from_env, pg, &perm_from);
//		cprintf("ns_output: after ipc recv %d\n", r);

		if (r < 0)
			panic("ns_output: Could not recv ipc %e", r);
		if (r != NSREQ_OUTPUT)
			panic("ns_output: Did not expect this ipc message");
		if (from_env != ns_envid)
			panic("ns_output: Wrong sender");
		if ((perm_from & PTE_P) == 0)
			panic("ns_output: received page is not PTE_P");

		struct jif_pkt *pkt = &nsipcbuf.pkt;
		if (pkt->jp_len > MAX_ETHERNET_PACKET_SIZE)
			panic("TODO handle this case later");

//		cprintf("ns_output: len = %x\n", pkt->jp_len);
//		cprintf("ns_output: bytes@%p -> ", &pkt->jp_data);

//		for (int i = 0; i < pkt->jp_len / sizeof(int); ++i)
//			cprintf("%x ", ((uint32_t *) pkt->jp_data)[i]);
//		cprintf("\n");

		// Try to transmit packet, until successful
		while (1)
		{
			r = sys_try_transmit_packet((uint8_t *) pkt->jp_data, pkt->jp_len);
			if (r == 0)
				break;
			else if (r == -E_INVAL)
				panic("ns_output: INVALID parameters");
			else if (r == -E_NET_QUEUE_FULL)
				sys_yield();
			else
				panic("ns_output: unknown error: %e\n", r);
		}
	}
}
