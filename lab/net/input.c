#include "ns.h"

#include <kern/e1000.h>

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

	int perm = PTE_P | PTE_W | PTE_U;
	size_t length;
	char pkt[PKT_BUF_SIZE];

	while (1) {
		while (sys_e1000_receive(pkt, &length) == -E_E1000_RXBUF_EMPTY)
			;

		int r;
		if ((r = sys_page_alloc(0, &nsipcbuf, perm)) < 0)
			panic("input: unable to allocate new page, error %e\n", r);

		memmove(nsipcbuf.pkt.jp_data, pkt, length);
		nsipcbuf.pkt.jp_len = length;

		ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, perm);
	}
}
