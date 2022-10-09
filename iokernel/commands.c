/*
 * commands.c - dataplane commands to/from runtimes
 */

#include <rte_mbuf.h>
#include <rte_ring.h>
#include <base/log.h>
#include <base/lrpc.h>
#include <iokernel/queue.h>

#include "defs.h"

static int commands_drain_queue(struct thread *t, struct rte_mbuf **bufs, int n)
{
	int i, n_bufs = 0;

	for (i = 0; i < n; i++) {
		uint64_t cmd;
		unsigned long payload;

		if (!lrpc_recv(&t->txcmdq, &cmd, &payload))
			break;

		switch (cmd) {
		case TXCMD_NET_COMPLETE:
			bufs[n_bufs++] = (struct rte_mbuf *)payload;
			/* TODO: validate pointer @buf */
			break;

		default:
			/* kill the runtime? */
			BUG();
		}
	}

	return n_bufs;
}

/*
 * Process a batch of commands from runtimes.
 */
bool commands_rx(void)
{
	struct tx_net_hdr *bufs[IOKERNEL_CMD_BURST_SIZE];
	int i, n_bufs = 0;
	static unsigned int pos = 0;

	/*
	 * Poll each thread in each runtime until all have been polled or we
	 * have processed CMD_BURST_SIZE commands.
	 */
	for (i = 0; i < nrts; i++) {
		unsigned int idx = (pos + i) % nrts;

		if (n_bufs >= IOKERNEL_CMD_BURST_SIZE){
			printf("commands_rx: n_bufs >= IOKERNEL_CMD_BURST_SIZE");
			break;
		}	
		n_bufs += commands_drain_queue(ts[idx], &bufs[n_bufs],
									   IOKERNEL_CMD_BURST_SIZE - n_bufs);
	}

	STAT_INC(COMMANDS_PULLED, n_bufs);

	pos++;
	int r=rte_ring_enqueue_burst(dp.completion_send_ring, (void **)bufs, n_bufs, NULL);
	if (r!=n_bufs) {
		printf("rte_ring_enqueue_burst failed, r=%d, n_bufs=%d", r, n_bufs);
	}
	n_bufs=rte_ring_dequeue_burst(dp.completion_recv_ring, (void **)bufs, IOKERNEL_CMD_BURST_SIZE,NULL);
	
	for (i = 0; i < n_bufs; i++)
	{
		tx_send_completion(bufs[i]);
	}
	return n_bufs > 0;
}
