#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	envid_t sender_envid;
	uint32_t req;
	int perm;

	while(1) {
		perm = 0;
		req = ipc_recv(&sender_envid, &nsipcbuf, &perm);

		if (((uint32_t *) sender_envid == 0) || (perm == 0)) {
			cprintf("Invalid request; ipc_recv encountered an error %e\n", req);
			continue;
		}

		if (sender_envid != ns_envid) {
			cprintf("Received IPC from envid %08x, expected to receive from %08x\n",
				sender_envid, ns_envid);
			continue;
		}

		if (sys_e1000_transmit(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len) == -E_E1000_TXBUF_FULL)
			cprintf("Dropping packet, after 20 tries cannot transmit");
	}
}
