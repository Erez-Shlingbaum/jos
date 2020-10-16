#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include "pci.h"

#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100e
#define MAX_ETHERNET_PACKET_SIZE 1518


int e1000_pci_attach(struct pci_func *pcif);
int e1000_try_transmit_packet(const uint8_t *packet_data, uint32_t packet_size);
int e1000_try_recv_packet(uint8_t *buffer, uint32_t buffer_size, uint32_t *packet_size);


/* Transmit descriptor */
union e1000_tx_desc {
	struct {
		uint64_t buffer_addr;       /* Address of the descriptor's data buffer */
		uint16_t length;    /* Data buffer length */
		uint8_t cso;        /* Checksum offset */
		uint8_t cmd;        /* Descriptor control */
		uint8_t status;     /* Descriptor status */
		uint8_t css;        /* Checksum start */
		uint16_t special;
	} fields;

	uint32_t raw;
};

union e1000_tctl_register {
	struct {
		uint32_t RST: 1;
		uint32_t EN: 1;
		uint32_t BCE: 1;
		uint32_t PSP: 1;
		uint32_t CT: 8;
		uint32_t COLD: 10;
		uint32_t SWXOFF: 1;
		uint32_t PBE: 1;
		uint32_t RTLC: 1;
		uint32_t NRTU: 1;
		uint32_t MULR: 1;
	} fields;

	uint32_t raw;
};

// Transmit inter packet gap register struct
// NOTE: This is not portable, relies on the hardware little-endianess
union e1000_tipg_register {
	struct {
		uint32_t IPGT: 10;
		uint32_t IPGR1: 10;
		uint32_t IPGR2: 10;
		uint32_t reserved: 2;
	} fields;

	uint32_t raw;
};

/* Receive descriptor */
struct e1000_rx_desc {
	uint64_t buffer_addr; /* Address of the descriptor's data buffer */
	uint16_t length;     /* Length of data DMAed into data buffer */
	uint16_t csum;       /* Packet checksum */
	uint8_t status;      /* Descriptor status */
	uint8_t errors;      /* Descriptor Errors */
	uint16_t special;
};

union e1000_rctl_register {
	struct {
		uint32_t reserved1: 1;
		uint32_t EN: 1;
		uint32_t SBP: 1;
		uint32_t UPE: 1;
		uint32_t MPE: 1;
		uint32_t LPE: 1;
		uint32_t LBM: 2;
		uint32_t RDMTS: 2;
		uint32_t reserved2: 2;
		uint32_t MO: 2;
		uint32_t reserved3: 1;
		uint32_t BAM: 1;
		uint32_t BSIZE: 2;
		uint32_t VFE: 1;
		uint32_t CFIEN: 1;
		uint32_t CFI: 1;
		uint32_t reserved4: 1;
		uint32_t DPF: 1;
		uint32_t PMCF: 1;
		uint32_t reserved5: 1;
		uint32_t BSEX: 1;
		uint32_t SECRC: 1;
		uint32_t reserved6: 5;
	} fields;

	uint32_t raw;
};

#define E1000_REG_INDEX(offset) (offset / sizeof(uint32_t))

// Transmit macros
#define E1000_TX_DESCRIPTORS_COUNT 64
#define E1000_TX_BUFFER_SIZE 2048
// Receive macros
#define E1000_RX_DESCRIPTORS_COUNT 128
#define E1000_RX_BUFFER_SIZE 2048


// Descriptor bits for both tx and dx
#define E1000_TX_DX_STAT_DD    0x01       // Descriptor Done

// Tx descriptor bits
#define E1000_TXD_CMD_RS     0x08   // Report Status
#define E1000_TXD_CMD_EOP    0x01   // End of Packet

/* Register Set. (82543, 82544)
 *
 * Registers are defined to be 32 bits and  should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the
 * host memory address space.
 *
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */
#define E1000_CTRL     E1000_REG_INDEX(0x00000)  /* Device Control - RW */
#define E1000_CTRL_DUP E1000_REG_INDEX(0x00004)  /* Device Control Duplicate (Shadow) - RW */
#define E1000_STATUS   E1000_REG_INDEX(0x00008)  /* Device Status - RO */
#define E1000_EECD     E1000_REG_INDEX(0x00010)  /* EEPROM/Flash Control - RW */
#define E1000_EERD     E1000_REG_INDEX(0x00014)  /* EEPROM Read - RW */
#define E1000_CTRL_EXT E1000_REG_INDEX(0x00018)  /* Extended Device Control - RW */
#define E1000_FLA      E1000_REG_INDEX(0x0001C)  /* Flash Access - RW */
#define E1000_MDIC     E1000_REG_INDEX(0x00020)  /* MDI Control - RW */
#define E1000_SCTL     E1000_REG_INDEX(0x00024)  /* SerDes Control - RW */
#define E1000_FEXTNVM  E1000_REG_INDEX(0x00028)  /* Future Extended NVM register */
#define E1000_FCAL     E1000_REG_INDEX(0x00028)  /* Flow Control Address Low - RW */
#define E1000_FCAH     E1000_REG_INDEX(0x0002C)  /* Flow Control Address High -RW */
#define E1000_FCT      E1000_REG_INDEX(0x00030)  /* Flow Control Type - RW */
#define E1000_VET      E1000_REG_INDEX(0x00038)  /* VLAN Ether Type - RW */
#define E1000_ICR      E1000_REG_INDEX(0x000C0)  /* Interrupt Cause Read - R/clr */
#define E1000_ITR      E1000_REG_INDEX(0x000C4)  /* Interrupt Throttling Rate - RW */
#define E1000_ICS      E1000_REG_INDEX(0x000C8)  /* Interrupt Cause Set - WO */
#define E1000_IMS      E1000_REG_INDEX(0x000D0)  /* Interrupt Mask Set - RW */
#define E1000_IMC      E1000_REG_INDEX(0x000D8)  /* Interrupt Mask Clear - WO */
#define E1000_IAM      E1000_REG_INDEX(0x000E0)  /* Interrupt Acknowledge Auto Mask */
#define E1000_RCTL     E1000_REG_INDEX(0x00100)  /* RX Control - RW */
#define E1000_RDTR1    E1000_REG_INDEX(0x02820)  /* RX Delay Timer (1) - RW */
#define E1000_RDBAL1   E1000_REG_INDEX(0x02900)  /* RX Descriptor Base Address Low (1) - RW */
#define E1000_RDBAH1   E1000_REG_INDEX(0x02904)  /* RX Descriptor Base Address High (1) - RW */
#define E1000_RDLEN1   E1000_REG_INDEX(0x02908)  /* RX Descriptor Length (1) - RW */
#define E1000_RDH1     E1000_REG_INDEX(0x02910)  /* RX Descriptor Head (1) - RW */
#define E1000_RDT1     E1000_REG_INDEX(0x02918)  /* RX Descriptor Tail (1) - RW */
#define E1000_FCTTV    E1000_REG_INDEX(0x00170)  /* Flow Control Transmit Timer Value - RW */
#define E1000_TXCW     E1000_REG_INDEX(0x00178)  /* TX Configuration Word - RW */
#define E1000_RXCW     E1000_REG_INDEX(0x00180)  /* RX Configuration Word - RO */
#define E1000_TCTL     E1000_REG_INDEX(0x00400)  /* TX Control - RW */
#define E1000_TCTL_EXT E1000_REG_INDEX(0x00404)  /* Extended TX Control - RW */
#define E1000_TIPG     E1000_REG_INDEX(0x00410)  /* TX Inter-packet gap -RW */
#define E1000_TBT      E1000_REG_INDEX(0x00448)  /* TX Burst Timer - RW */
#define E1000_AIT      E1000_REG_INDEX(0x00458)  /* Adaptive Interframe Spacing Throttle - RW */
#define E1000_LEDCTL   E1000_REG_INDEX(0x00E00)  /* LED Control - RW */
#define E1000_EXTCNF_CTRL  E1000_REG_INDEX(0x00F00)  /* Extended Configuration Control */
#define E1000_EXTCNF_SIZE  E1000_REG_INDEX(0x00F08)  /* Extended Configuration Size */
#define E1000_PHY_CTRL     E1000_REG_INDEX(0x00F10)  /* PHY Control Register in CSR */
#define FEXTNVM_SW_CONFIG  E1000_REG_INDEX(0x0001)
#define E1000_PBA      E1000_REG_INDEX(0x01000)  /* Packet Buffer Allocation - RW */
#define E1000_PBS      E1000_REG_INDEX(0x01008)  /* Packet Buffer Size */
#define E1000_EEMNGCTL E1000_REG_INDEX(0x01010)  /* MNG EEprom Control */
#define E1000_FLASH_UPDATES E1000_REG_INDEX(1000)
#define E1000_EEARBC   E1000_REG_INDEX(0x01024)  /* EEPROM Auto Read Bus Control */
#define E1000_FLASHT   E1000_REG_INDEX(0x01028)  /* FLASH Timer Register */
#define E1000_EEWR     E1000_REG_INDEX(0x0102C)  /* EEPROM Write Register - RW */
#define E1000_FLSWCTL  E1000_REG_INDEX(0x01030)  /* FLASH control register */
#define E1000_FLSWDATA E1000_REG_INDEX(0x01034)  /* FLASH data register */
#define E1000_FLSWCNT  E1000_REG_INDEX(0x01038)  /* FLASH Access Counter */
#define E1000_FLOP     E1000_REG_INDEX(0x0103C)  /* FLASH Opcode Register */
#define E1000_ERT      E1000_REG_INDEX(0x02008)  /* Early Rx Threshold - RW */
#define E1000_FCRTL    E1000_REG_INDEX(0x02160)  /* Flow Control Receive Threshold Low - RW */
#define E1000_FCRTH    E1000_REG_INDEX(0x02168)  /* Flow Control Receive Threshold High - RW */
#define E1000_PSRCTL   E1000_REG_INDEX(0x02170)  /* Packet Split Receive Control - RW */
#define E1000_RDBAL    E1000_REG_INDEX(0x02800)  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    E1000_REG_INDEX(0x02804)  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    E1000_REG_INDEX(0x02808)  /* RX Descriptor Length - RW */
#define E1000_RDH      E1000_REG_INDEX(0x02810)  /* RX Descriptor Head - RW */
#define E1000_RDT      E1000_REG_INDEX(0x02818)  /* RX Descriptor Tail - RW */
#define E1000_RDTR     E1000_REG_INDEX(0x02820)  /* RX Delay Timer - RW */
#define E1000_RDBAL0   E1000_REG_INDEX(E1000_RDBAL) /* RX Desc Base Address Low (0) - RW */
#define E1000_RDBAH0   E1000_REG_INDEX(E1000_RDBAH) /* RX Desc Base Address High (0) - RW */
#define E1000_RDLEN0   E1000_REG_INDEX(E1000_RDLEN) /* RX Desc Length (0) - RW */
#define E1000_RDH0     E1000_REG_INDEX(E1000_RDH)   /* RX Desc Head (0) - RW */
#define E1000_RDT0     E1000_REG_INDEX(E1000_RDT)   /* RX Desc Tail (0) - RW */
#define E1000_RDTR0    E1000_REG_INDEX(E1000_RDTR)  /* RX Delay Timer (0) - RW */
#define E1000_RXDCTL   E1000_REG_INDEX(0x02828)  /* RX Descriptor Control queue 0 - RW */
#define E1000_RXDCTL1  E1000_REG_INDEX(0x02928)  /* RX Descriptor Control queue 1 - RW */
#define E1000_RADV     E1000_REG_INDEX(0x0282C)  /* RX Interrupt Absolute Delay Timer - RW */
#define E1000_RSRPD    E1000_REG_INDEX(0x02C00)  /* RX Small Packet Detect - RW */
#define E1000_RAID     E1000_REG_INDEX(0x02C08)  /* Receive Ack Interrupt Delay - RW */
#define E1000_TXDMAC   E1000_REG_INDEX(0x03000)  /* TX DMA Control - RW */
#define E1000_KABGTXD  E1000_REG_INDEX(0x03004)  /* AFE Band Gap Transmit Ref Data */
#define E1000_TDFH     E1000_REG_INDEX(0x03410)  /* TX Data FIFO Head - RW */
#define E1000_TDFT     E1000_REG_INDEX(0x03418)  /* TX Data FIFO Tail - RW */
#define E1000_TDFHS    E1000_REG_INDEX(0x03420)  /* TX Data FIFO Head Saved - RW */
#define E1000_TDFTS    E1000_REG_INDEX(0x03428)  /* TX Data FIFO Tail Saved - RW */
#define E1000_TDFPC    E1000_REG_INDEX(0x03430)  /* TX Data FIFO Packet Count - RW */
#define E1000_TDBAL    E1000_REG_INDEX(0x03800)  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    E1000_REG_INDEX(0x03804)  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    E1000_REG_INDEX(0x03808)  /* TX Descriptor Length - RW */
#define E1000_TDH      E1000_REG_INDEX(0x03810)  /* TX Descriptor Head - RW */
#define E1000_TDT      E1000_REG_INDEX(0x03818)  /* TX Descripotr Tail - RW */
#define E1000_TIDV     E1000_REG_INDEX(0x03820)  /* TX Interrupt Delay Value - RW */
#define E1000_TXDCTL   E1000_REG_INDEX(0x03828)  /* TX Descriptor Control - RW */
#define E1000_TADV     E1000_REG_INDEX(0x0382C)  /* TX Interrupt Absolute Delay Val - RW */
#define E1000_TSPMT    E1000_REG_INDEX(0x03830)  /* TCP Segmentation PAD & Min Threshold - RW */
#define E1000_TARC0    E1000_REG_INDEX(0x03840)  /* TX Arbitration Count (0) */
#define E1000_TDBAL1   E1000_REG_INDEX(0x03900)  /* TX Desc Base Address Low (1) - RW */
#define E1000_TDBAH1   E1000_REG_INDEX(0x03904)  /* TX Desc Base Address High (1) - RW */
#define E1000_TDLEN1   E1000_REG_INDEX(0x03908)  /* TX Desc Length (1) - RW */
#define E1000_TDH1     E1000_REG_INDEX(0x03910)  /* TX Desc Head (1) - RW */
#define E1000_TDT1     E1000_REG_INDEX(0x03918)  /* TX Desc Tail (1) - RW */
#define E1000_TXDCTL1  E1000_REG_INDEX(0x03928)  /* TX Descriptor Control (1) - RW */
#define E1000_TARC1    E1000_REG_INDEX(0x03940)  /* TX Arbitration Count (1) */
#define E1000_CRCERRS  E1000_REG_INDEX(0x04000)  /* CRC Error Count - R/clr */
#define E1000_ALGNERRC E1000_REG_INDEX(0x04004)  /* Alignment Error Count - R/clr */
#define E1000_SYMERRS  E1000_REG_INDEX(0x04008)  /* Symbol Error Count - R/clr */
#define E1000_RXERRC   E1000_REG_INDEX(0x0400C)  /* Receive Error Count - R/clr */
#define E1000_MPC      E1000_REG_INDEX(0x04010)  /* Missed Packet Count - R/clr */
#define E1000_SCC      E1000_REG_INDEX(0x04014)  /* Single Collision Count - R/clr */
#define E1000_ECOL     E1000_REG_INDEX(0x04018)  /* Excessive Collision Count - R/clr */
#define E1000_MCC      E1000_REG_INDEX(0x0401C)  /* Multiple Collision Count - R/clr */
#define E1000_LATECOL  E1000_REG_INDEX(0x04020)  /* Late Collision Count - R/clr */
#define E1000_COLC     E1000_REG_INDEX(0x04028)  /* Collision Count - R/clr */
#define E1000_DC       E1000_REG_INDEX(0x04030)  /* Defer Count - R/clr */
#define E1000_TNCRS    E1000_REG_INDEX(0x04034)  /* TX-No CRS - R/clr */
#define E1000_SEC      E1000_REG_INDEX(0x04038)  /* Sequence Error Count - R/clr */
#define E1000_CEXTERR  E1000_REG_INDEX(0x0403C)  /* Carrier Extension Error Count - R/clr */
#define E1000_RLEC     E1000_REG_INDEX(0x04040)  /* Receive Length Error Count - R/clr */
#define E1000_XONRXC   E1000_REG_INDEX(0x04048)  /* XON RX Count - R/clr */
#define E1000_XONTXC   E1000_REG_INDEX(0x0404C)  /* XON TX Count - R/clr */
#define E1000_XOFFRXC  E1000_REG_INDEX(0x04050)  /* XOFF RX Count - R/clr */
#define E1000_XOFFTXC  E1000_REG_INDEX(0x04054)  /* XOFF TX Count - R/clr */
#define E1000_FCRUC    E1000_REG_INDEX(0x04058)  /* Flow Control RX Unsupported Count- R/clr */
#define E1000_PRC64    E1000_REG_INDEX(0x0405C)  /* Packets RX (64 bytes) - R/clr */
#define E1000_PRC127   E1000_REG_INDEX(0x04060)  /* Packets RX (65-127 bytes) - R/clr */
#define E1000_PRC255   E1000_REG_INDEX(0x04064)  /* Packets RX (128-255 bytes) - R/clr */
#define E1000_PRC511   E1000_REG_INDEX(0x04068)  /* Packets RX (255-511 bytes) - R/clr */
#define E1000_PRC1023  E1000_REG_INDEX(0x0406C)  /* Packets RX (512-1023 bytes) - R/clr */
#define E1000_PRC1522  E1000_REG_INDEX(0x04070)  /* Packets RX (1024-1522 bytes) - R/clr */
#define E1000_GPRC     E1000_REG_INDEX(0x04074)  /* Good Packets RX Count - R/clr */
#define E1000_BPRC     E1000_REG_INDEX(0x04078)  /* Broadcast Packets RX Count - R/clr */
#define E1000_MPRC     E1000_REG_INDEX(0x0407C)  /* Multicast Packets RX Count - R/clr */
#define E1000_GPTC     E1000_REG_INDEX(0x04080)  /* Good Packets TX Count - R/clr */
#define E1000_GORCL    E1000_REG_INDEX(0x04088)  /* Good Octets RX Count Low - R/clr */
#define E1000_GORCH    E1000_REG_INDEX(0x0408C)  /* Good Octets RX Count High - R/clr */
#define E1000_GOTCL    E1000_REG_INDEX(0x04090)  /* Good Octets TX Count Low - R/clr */
#define E1000_GOTCH    E1000_REG_INDEX(0x04094)  /* Good Octets TX Count High - R/clr */
#define E1000_RNBC     E1000_REG_INDEX(0x040A0)  /* RX No Buffers Count - R/clr */
#define E1000_RUC      E1000_REG_INDEX(0x040A4)  /* RX Undersize Count - R/clr */
#define E1000_RFC      E1000_REG_INDEX(0x040A8)  /* RX Fragment Count - R/clr */
#define E1000_ROC      E1000_REG_INDEX(0x040AC)  /* RX Oversize Count - R/clr */
#define E1000_RJC      E1000_REG_INDEX(0x040B0)  /* RX Jabber Count - R/clr */
#define E1000_MGTPRC   E1000_REG_INDEX(0x040B4)  /* Management Packets RX Count - R/clr */
#define E1000_MGTPDC   E1000_REG_INDEX(0x040B8)  /* Management Packets Dropped Count - R/clr */
#define E1000_MGTPTC   E1000_REG_INDEX(0x040BC)  /* Management Packets TX Count - R/clr */
#define E1000_TORL     E1000_REG_INDEX(0x040C0)  /* Total Octets RX Low - R/clr */
#define E1000_TORH     E1000_REG_INDEX(0x040C4)  /* Total Octets RX High - R/clr */
#define E1000_TOTL     E1000_REG_INDEX(0x040C8)  /* Total Octets TX Low - R/clr */
#define E1000_TOTH     E1000_REG_INDEX(0x040CC)  /* Total Octets TX High - R/clr */
#define E1000_TPR      E1000_REG_INDEX(0x040D0)  /* Total Packets RX - R/clr */
#define E1000_TPT      E1000_REG_INDEX(0x040D4)  /* Total Packets TX - R/clr */
#define E1000_PTC64    E1000_REG_INDEX(0x040D8)  /* Packets TX (64 bytes) - R/clr */
#define E1000_PTC127   E1000_REG_INDEX(0x040DC)  /* Packets TX (65-127 bytes) - R/clr */
#define E1000_PTC255   E1000_REG_INDEX(0x040E0)  /* Packets TX (128-255 bytes) - R/clr */
#define E1000_PTC511   E1000_REG_INDEX(0x040E4)  /* Packets TX (256-511 bytes) - R/clr */
#define E1000_PTC1023  E1000_REG_INDEX(0x040E8)  /* Packets TX (512-1023 bytes) - R/clr */
#define E1000_PTC1522  E1000_REG_INDEX(0x040EC)  /* Packets TX (1024-1522 Bytes) - R/clr */
#define E1000_MPTC     E1000_REG_INDEX(0x040F0)  /* Multicast Packets TX Count - R/clr */
#define E1000_BPTC     E1000_REG_INDEX(0x040F4)  /* Broadcast Packets TX Count - R/clr */
#define E1000_TSCTC    E1000_REG_INDEX(0x040F8)  /* TCP Segmentation Context TX - R/clr */
#define E1000_TSCTFC   E1000_REG_INDEX(0x040FC)  /* TCP Segmentation Context TX Fail - R/clr */
#define E1000_IAC      E1000_REG_INDEX(0x04100)  /* Interrupt Assertion Count */
#define E1000_ICRXPTC  E1000_REG_INDEX(0x04104)  /* Interrupt Cause Rx Packet Timer Expire Count */
#define E1000_ICRXATC  E1000_REG_INDEX(0x04108)  /* Interrupt Cause Rx Absolute Timer Expire Count */
#define E1000_ICTXPTC  E1000_REG_INDEX(0x0410C)  /* Interrupt Cause Tx Packet Timer Expire Count */
#define E1000_ICTXATC  E1000_REG_INDEX(0x04110)  /* Interrupt Cause Tx Absolute Timer Expire Count */
#define E1000_ICTXQEC  E1000_REG_INDEX(0x04118)  /* Interrupt Cause Tx Queue Empty Count */
#define E1000_ICTXQMTC E1000_REG_INDEX(0x0411C)  /* Interrupt Cause Tx Queue Minimum Threshold Count */
#define E1000_ICRXDMTC E1000_REG_INDEX(0x04120)  /* Interrupt Cause Rx Descriptor Minimum Threshold Count */
#define E1000_ICRXOC   E1000_REG_INDEX(0x04124)  /* Interrupt Cause Receiver Overrun Count */
#define E1000_RXCSUM   E1000_REG_INDEX(0x05000)  /* RX Checksum Control - RW */
#define E1000_RFCTL    E1000_REG_INDEX(0x05008)  /* Receive Filter Control*/
#define E1000_MTA      E1000_REG_INDEX(0x05200)  /* Multicast Table Array - RW Array */
#define E1000_RA       E1000_REG_INDEX(0x05400)  /* Receive Address - RW Array */
#define E1000_VFTA     E1000_REG_INDEX(0x05600)  /* VLAN Filter Table Array - RW Array */
#define E1000_WUC      E1000_REG_INDEX(0x05800)  /* Wakeup Control - RW */
#define E1000_WUFC     E1000_REG_INDEX(0x05808)  /* Wakeup Filter Control - RW */
#define E1000_WUS      E1000_REG_INDEX(0x05810)  /* Wakeup Status - RO */
#define E1000_MANC     E1000_REG_INDEX(0x05820)  /* Management Control - RW */
#define E1000_IPAV     E1000_REG_INDEX(0x05838)  /* IP Address Valid - RW */
#define E1000_IP4AT    E1000_REG_INDEX(0x05840)  /* IPv4 Address Table - RW Array */
#define E1000_IP6AT    E1000_REG_INDEX(0x05880)  /* IPv6 Address Table - RW Array */
#define E1000_WUPL     E1000_REG_INDEX(0x05900)  /* Wakeup Packet Length - RW */
#define E1000_WUPM     E1000_REG_INDEX(0x05A00)  /* Wakeup Packet Memory - RO A */
#define E1000_FFLT     E1000_REG_INDEX(0x05F00)  /* Flexible Filter Length Table - RW Array */
#define E1000_HOST_IF  E1000_REG_INDEX(0x08800)  /* Host Interface */
#define E1000_FFMT     E1000_REG_INDEX(0x09000)  /* Flexible Filter Mask Table - RW Array */
#define E1000_FFVT     E1000_REG_INDEX(0x09800)  /* Flexible Filter Value Table - RW Array */

#define E1000_KUMCTRLSTA E1000_REG_INDEX(0x00034) /* MAC-PHY interface - RW */
#define E1000_MDPHYA     E1000_REG_INDEX(0x0003C)  /* PHY address - RW */
#define E1000_MANC2H     E1000_REG_INDEX(0x05860)  /* Managment Control To Host - RW */
#define E1000_SW_FW_SYNC E1000_REG_INDEX(0x05B5C) /* Software-Firmware Synchronization - RW */

#define E1000_GCR       E1000_REG_INDEX(0x05B00) /* PCI-Ex Control */
#define E1000_GSCL_1    E1000_REG_INDEX(0x05B10) /* PCI-Ex Statistic Control #1 */
#define E1000_GSCL_2    E1000_REG_INDEX(0x05B14) /* PCI-Ex Statistic Control #2 */
#define E1000_GSCL_3    E1000_REG_INDEX(0x05B18) /* PCI-Ex Statistic Control #3 */
#define E1000_GSCL_4    E1000_REG_INDEX(0x05B1C) /* PCI-Ex Statistic Control #4 */
#define E1000_FACTPS    E1000_REG_INDEX(0x05B30) /* Function Active and Power State to MNG */
#define E1000_SWSM      E1000_REG_INDEX(0x05B50) /* SW Semaphore */
#define E1000_FWSM      E1000_REG_INDEX(0x05B54) /* FW Semaphore */
#define E1000_FFLT_DBG  E1000_REG_INDEX(0x05F04) /* Debug Register */
#define E1000_HICR      E1000_REG_INDEX(0x08F00) /* Host Inteface Control */

/* Transmit Control */
#define E1000_TCTL_RST    0x00000001    /* software reset */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
#define E1000_TCTL_MULR   0x10000000    /* Multiple request support */

#endif  // SOL >= 6