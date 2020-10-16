#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

static volatile uint32_t *e1000_dma_io = NULL;

struct tx_buffer {
	char data[E1000_TX_BUFFER_SIZE];
};
struct rx_buffer {
	char data[E1000_RX_BUFFER_SIZE];
};

// Transmit:
union e1000_tx_desc tx_descriptor_ring[E1000_TX_DESCRIPTORS_COUNT];
struct tx_buffer tx_buffers[E1000_TX_DESCRIPTORS_COUNT];

// Receive:
struct e1000_rx_desc receive_descriptor_ring[E1000_RX_DESCRIPTORS_COUNT];
struct rx_buffer receive_buffers[E1000_RX_DESCRIPTORS_COUNT];

static void e1000_init_transmit_ring()
{
	// Set base address and size of descriptor ring
	e1000_dma_io[E1000_TDBAL] = PADDR((void *) tx_descriptor_ring);
	e1000_dma_io[E1000_TDBAH] = 0;
	e1000_dma_io[E1000_TDLEN] = sizeof(tx_descriptor_ring);

	// Initialize ring
	for (int i = 0; i < E1000_TX_DESCRIPTORS_COUNT; ++i)
	{
		tx_descriptor_ring[i].fields.buffer_addr = PADDR((void *) &tx_buffers[i]);
		// Report status, end of packet
		tx_descriptor_ring[i].fields.cmd |= E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;
		// Turn DD(descriptor done) bit(index 0), to let the transmit function know it is safe to recycle
		tx_descriptor_ring[i].fields.status |= E1000_TX_DX_STAT_DD;
	}

	// Set TDH, TDT to 0
	e1000_dma_io[E1000_TDH] = 0;
	e1000_dma_io[E1000_TDT] = 0;

	// Set TCTL register for desired operation
	// Set enable bit, pad short packets, collision threshold to 0x10 (TODO remove this later if we don't need it), and collision distance to full duplex
	e1000_dma_io[E1000_TCTL] =
			((union e1000_tctl_register) {.fields.EN = 1, .fields.PSP = 1, .fields.CT = 0x10, .fields.COLD=0x40}).raw;

	e1000_dma_io[E1000_TIPG] =
			((union e1000_tipg_register) {.fields.IPGT = 8, .fields.IPGR1 = 0, .fields.IPGR2=0}).raw;
}

static void e1000_init_receive_ring()
{
//#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */

	// Set base address and size of descriptor ring
	// Hard code QEMU's mac address 52:54:00:12:34:56
	// Low 32 bits 52:54:00:12
//	e1000_dma_io[E1000_RA] = 0x12005452;
//	*(uint16_t *) &e1000_dma_io[E1000_RA + 1] = 0x5634;
//	e1000_dma_io[E1000_RA + 1] |= E1000_RAH_AV;

	// Clear MTA
	e1000_dma_io[E1000_MTA] = 0;

	// Initialize ring pointers
	e1000_dma_io[E1000_RDBAL] = PADDR((void *) receive_descriptor_ring);
	e1000_dma_io[E1000_RDBAH] = 0;
	e1000_dma_io[E1000_RDLEN] = sizeof(receive_descriptor_ring);

	// Set RDH, RDT to 0
	e1000_dma_io[E1000_RDH] = 0;
	e1000_dma_io[E1000_RDT] = E1000_RX_DESCRIPTORS_COUNT - 1; // TODO check this, might be pointers?

	// Initialize ring descriptors
	for (int i = 0; i < E1000_RX_DESCRIPTORS_COUNT; ++i)
	{
		receive_descriptor_ring[i].buffer_addr = PADDR(&receive_buffers[i]);
		receive_descriptor_ring[i].status &= ~E1000_TX_DX_STAT_DD;
	}

	// Set RCTL register for desired operation
	e1000_dma_io[E1000_RCTL] =
			((union e1000_rctl_register) {.fields.EN = 1, .fields.BAM=1, .fields.BSIZE = 0b00 /* 2048 */, .fields.SECRC = 1}).raw;
}


int e1000_pci_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);
	e1000_dma_io = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	// indicates a full duplex link is up at 1000 MB/s, among other things.
	assert(e1000_dma_io[E1000_STATUS] == 0x80080783);

	e1000_init_transmit_ring();
	e1000_init_receive_ring();
	return 0;
}

int e1000_try_transmit_packet(const uint8_t *packet_data, uint32_t packet_size)
{
//	cprintf("e1000_try_transmit_packet: args %p %d\n", packet_data, packet_size);
	if (packet_size > E1000_TX_BUFFER_SIZE)
		return -E_INVAL;

	uint32_t tail_index = e1000_dma_io[E1000_TDT];
	volatile union e1000_tx_desc *current_descriptor = &tx_descriptor_ring[tail_index];

//	cprintf("e1000_try_transmit_packet: *tdt_reg = %x\n", tail_index);

	// DD bit is the 0th bit) - Is it safe to recycle this buffer?
	// If DD bit is not set
	if ((tx_descriptor_ring[tail_index].fields.status & E1000_TX_DX_STAT_DD) == 0)
		return -E_NET_QUEUE_FULL;

	// Turn off the DD bit(index 0)
	current_descriptor->fields.status &= ~E1000_TX_DX_STAT_DD;

	current_descriptor->fields.length = packet_size;
	// Copy packet data to buffer
	memcpy((void *) &tx_buffers[tail_index], packet_data, packet_size);

//	cprintf("e1000_try_transmit_packet: send hex dump -> ");
//	for (int i = 0; i < current_descriptor->fields.length / sizeof(int); ++i)
//		cprintf("%x ", ((uint32_t *) tx_buffers[tail_index].data)[i]);
//	cprintf("\n");

	// Advance the tail
	e1000_dma_io[E1000_TDT] = (tail_index + 1) % E1000_TX_DESCRIPTORS_COUNT;
	return 0;
}

int e1000_try_recv_packet(uint8_t *buffer, uint32_t buffer_size, uint32_t *packet_size)
{
//	cprintf("e1000_try_recv_packet: args %p %x %p\n", buffer, buffer_size, packet_size);

	uint32_t tail_index = (e1000_dma_io[E1000_RDT] + 1) % E1000_RX_DESCRIPTORS_COUNT; // We are checking next in queue
	volatile struct e1000_rx_desc *current_descriptor = &receive_descriptor_ring[tail_index];

	// If DD bit is not set, then the recv queue has nothing for us to recv
	if ((current_descriptor->status & E1000_TX_DX_STAT_DD) == 0)
		return -E_NET_QUEUE_EMPTY;

	// Even if the user supplied a small buffer, we will write to him the size of packet needed
	if (packet_size != NULL)
		*packet_size = current_descriptor->length;

	if (current_descriptor->length > buffer_size)
		return -E_INVAL;

//	cprintf("e1000_try_recv_packet: hex dump -> ");
//	for (int i = 0; i < current_descriptor->length / sizeof(int); ++i)
//		cprintf("%x ", ((uint32_t *) receive_buffers[tail_index].data)[i]);
//	cprintf("\n");
//	cprintf("LOLOLOLO RECV: %d\n", current_descriptor->length);

	// Copy packet data to buffer
	memcpy((void *) buffer, (void *) &receive_buffers[tail_index], current_descriptor->length);

	// Mark this entry as done
	current_descriptor->status &= ~E1000_TX_DX_STAT_DD;

	// Advance the tail
	e1000_dma_io[E1000_RDT] = tail_index;
	return 0;
}