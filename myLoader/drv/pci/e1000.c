#include "public.h"
#include "pci.h"
#include "module.h"
#include "interrupt.h"
#include "net.h"
#include "io.h"
#include "memory.h"
#include "ml_string.h"
#include "debug.h"
#include "command.h"
#include "task.h"

/* some pci vendor device */
#define PCI_VENDOR_ID_INTEL			0x8086
#define PCI_DEVICE_ID_INTEL_82540EM	0x100E
#define PCI_DEVICE_ID_INTEL_82545EM	0x100F

static const u32 e1000_macaddr[2] = {0x5254, 0x00123456};

/* INT FLAG */
#define	E1000INTFLAG_RXT0			(0x00000080)

/* register list */
#define E1000REG_CTRL				(0x0000)
#define E1000REG_CTRL_FD			(0x1 << 0)
#define E1000REG_CTRL_LRST			(0x1 << 3)		/* link reset */
#define E1000REG_CTRL_ASDE			(0x1 << 5)		/* autospeed detection enable */
#define E1000REG_CTRL_SLU			(0x1 << 6)		/* set link up */
#define E1000REG_CTRL_ILOS			(0x1 << 7)
#define E1000REG_CTRL_FRCSPD		(0x1 << 11)
#define E1000REG_CTRL_FRCDPLX		(0x1 << 12)
#define E1000_CTRL_SWDPIN0  0x00040000	/* SWDPIN 0 value */
#define E1000_CTRL_SWDPIN1  0x00080000	/* SWDPIN 1 value */
#define E1000_CTRL_SWDPIN2  0x00100000	/* SWDPIN 2 value */
#define E1000_CTRL_SWDPIN3  0x00200000	/* SWDPIN 3 value */
#define E1000_CTRL_SWDPIO0  0x00400000	/* SWDPIN 0 Input or output */
#define E1000_CTRL_SWDPIO1  0x00800000	/* SWDPIN 1 input or output */
#define E1000_CTRL_SWDPIO2  0x01000000	/* SWDPIN 2 input or output */
#define E1000_CTRL_SWDPIO3  0x02000000	/* SWDPIN 3 input or output */
#define E1000REG_CTRL_RST			(0x1 << 26)
#define E1000REG_CTRL_PHY_RST		(0x1 << 31)

#define E1000REG_STATUS				(0x0008)
#define E1000REG_STATUS_LU			(0x1 << 1)

#define E1000REG_EECD				0x00010	/* EEPROM/Flash Control - RW */

#define E1000REG_MDIC				(0x0020)
#define E1000_MDIC_DATA_MASK 0x0000FFFF
#define E1000_MDIC_REG_MASK  0x001F0000
#define E1000_MDIC_REG_SHIFT 16
#define E1000_MDIC_PHY_MASK  0x03E00000
#define E1000_MDIC_PHY_SHIFT 21
#define E1000_MDIC_OP_WRITE  0x04000000
#define E1000_MDIC_OP_READ   0x08000000
#define E1000_MDIC_READY     0x10000000
#define E1000_MDIC_INT_EN    0x20000000
#define E1000_MDIC_ERROR     0x40000000

#define E1000REG_FCAL     0x00028	/* Flow Control Address Low - RW */
#define E1000REG_FCAH     0x0002C	/* Flow Control Address High -RW */
#define E1000REG_FCT      0x00030	/* Flow Control Type - RW */

#define E1000REG_VET		(0x0038)

#define E1000REG_ICR		(0x00C0)
#define E1000REG_ITR		(0x00C4)
#define E1000REG_ICS		(0x00C8)
#define E1000REG_IMS		(0x00D0)
#define E1000REG_IMC		(0x00D8)

#define E1000REG_PBA      0x01000	/* Packet Buffer Allocation - RW */
#define E1000REG_PBS      0x01008	/* Packet Buffer Size */

/* PHY 1000 MII Register/Bit Definitions */
/* PHY Registers defined by IEEE */
#define PHY_CTRL         0x00	/* Control Register */
#define PHY_STATUS       0x01	/* Status Register */
#define PHY_ID1          0x02	/* Phy Id Reg (word 1) */
#define PHY_ID2          0x03	/* Phy Id Reg (word 2) */
#define PHY_AUTONEG_ADV  0x04	/* Autoneg Advertisement */
#define PHY_LP_ABILITY   0x05	/* Link Partner Ability (Base Page) */
#define PHY_AUTONEG_EXP  0x06	/* Autoneg Expansion Reg */
#define PHY_NEXT_PAGE_TX 0x07	/* Next Page TX */
#define PHY_LP_NEXT_PAGE 0x08	/* Link Partner Next Page */
#define PHY_1000T_CTRL   0x09	/* 1000Base-T Control Reg */
#define PHY_1000T_STATUS 0x0A	/* 1000Base-T Status Reg */
#define PHY_EXT_STATUS   0x0F	/* Extended Status Reg */

#define MAX_PHY_REG_ADDRESS        0x1F	/* 5 bit address bus (0-0x1F) */
#define MAX_PHY_MULTI_PAGE_REG     0xF	/* Registers equal on all pages */

/* M88E1000 Specific Registers */
#define M88E1000_PHY_SPEC_CTRL     0x10	/* PHY Specific Control Register */
#define M88E1000_PHY_SPEC_STATUS   0x11	/* PHY Specific Status Register */
#define M88E1000_INT_ENABLE        0x12	/* Interrupt Enable Register */
#define M88E1000_INT_STATUS        0x13	/* Interrupt Status Register */
#define M88E1000_EXT_PHY_SPEC_CTRL 0x14	/* Extended PHY Specific Control */
#define M88E1000_RX_ERR_CNTR       0x15	/* Receive Error Counter */

#define M88E1000_PHY_EXT_CTRL      0x1A	/* PHY extend control register */
#define M88E1000_PHY_PAGE_SELECT   0x1D	/* Reg 29 for page number setting */
#define M88E1000_PHY_GEN_CONTROL   0x1E	/* Its meaning depends on reg 29 */
#define M88E1000_PHY_VCO_REG_BIT8  0x100	/* Bits 8 & 11 are adjusted for */
#define M88E1000_PHY_VCO_REG_BIT11 0x800	/* improved BER performance */

#define IGP01E1000_IEEE_REGS_PAGE  0x0000
#define IGP01E1000_IEEE_RESTART_AUTONEG 0x3300
#define IGP01E1000_IEEE_FORCE_GIGA      0x0140

/* IGP01E1000 Specific Registers */
#define IGP01E1000_PHY_PORT_CONFIG 0x10	/* PHY Specific Port Config Register */
#define IGP01E1000_PHY_PORT_STATUS 0x11	/* PHY Specific Status Register */
#define IGP01E1000_PHY_PORT_CTRL   0x12	/* PHY Specific Control Register */
#define IGP01E1000_PHY_LINK_HEALTH 0x13	/* PHY Link Health Register */
#define IGP01E1000_GMII_FIFO       0x14	/* GMII FIFO Register */
#define IGP01E1000_PHY_CHANNEL_QUALITY 0x15	/* PHY Channel Quality Register */
#define IGP02E1000_PHY_POWER_MGMT      0x19
#define IGP01E1000_PHY_PAGE_SELECT     0x1F	/* PHY Page Select Core Register */

/* IGP01E1000 AGC Registers - stores the cable length values*/
#define IGP01E1000_PHY_AGC_A        0x1172
#define IGP01E1000_PHY_AGC_B        0x1272
#define IGP01E1000_PHY_AGC_C        0x1472
#define IGP01E1000_PHY_AGC_D        0x1872

/* IGP02E1000 AGC Registers for cable length values */
#define IGP02E1000_PHY_AGC_A        0x11B1
#define IGP02E1000_PHY_AGC_B        0x12B1
#define IGP02E1000_PHY_AGC_C        0x14B1
#define IGP02E1000_PHY_AGC_D        0x18B1

/* IGP01E1000 DSP Reset Register */
#define IGP01E1000_PHY_DSP_RESET   0x1F33
#define IGP01E1000_PHY_DSP_SET     0x1F71
#define IGP01E1000_PHY_DSP_FFE     0x1F35

#define IGP01E1000_PHY_CHANNEL_NUM    4
#define IGP02E1000_PHY_CHANNEL_NUM    4

#define IGP01E1000_PHY_AGC_PARAM_A    0x1171
#define IGP01E1000_PHY_AGC_PARAM_B    0x1271
#define IGP01E1000_PHY_AGC_PARAM_C    0x1471
#define IGP01E1000_PHY_AGC_PARAM_D    0x1871

#define IGP01E1000_PHY_EDAC_MU_INDEX        0xC000
#define IGP01E1000_PHY_EDAC_SIGN_EXT_9_BITS 0x8000

#define IGP01E1000_PHY_ANALOG_TX_STATE      0x2890
#define IGP01E1000_PHY_ANALOG_CLASS_A       0x2000
#define IGP01E1000_PHY_FORCE_ANALOG_ENABLE  0x0004
#define IGP01E1000_PHY_DSP_FFE_CM_CP        0x0069

#define IGP01E1000_PHY_DSP_FFE_DEFAULT      0x002A
/* IGP01E1000 PCS Initialization register - stores the polarity status when
 * speed = 1000 Mbps. */
#define IGP01E1000_PHY_PCS_INIT_REG  0x00B4
#define IGP01E1000_PHY_PCS_CTRL_REG  0x00B5

#define IGP01E1000_ANALOG_REGS_PAGE  0x20C0

/* PHY Control Register */
#define MII_CR_SPEED_SELECT_MSB 0x0040	/* bits 6,13: 10=1000, 01=100, 00=10 */
#define MII_CR_COLL_TEST_ENABLE 0x0080	/* Collision test enable */
#define MII_CR_FULL_DUPLEX      0x0100	/* FDX =1, half duplex =0 */
#define MII_CR_RESTART_AUTO_NEG 0x0200	/* Restart auto negotiation */
#define MII_CR_ISOLATE          0x0400	/* Isolate PHY from MII */
#define MII_CR_POWER_DOWN       0x0800	/* Power down */
#define MII_CR_AUTO_NEG_EN      0x1000	/* Auto Neg Enable */
#define MII_CR_SPEED_SELECT_LSB 0x2000	/* bits 6,13: 10=1000, 01=100, 00=10 */
#define MII_CR_LOOPBACK         0x4000	/* 0 = normal, 1 = loopback */
#define MII_CR_RESET            0x8000	/* 0 = normal, 1 = PHY reset */

/* PHY Status Register */
#define MII_SR_EXTENDED_CAPS     0x0001	/* Extended register capabilities */
#define MII_SR_JABBER_DETECT     0x0002	/* Jabber Detected */
#define MII_SR_LINK_STATUS       0x0004	/* Link Status 1 = link */
#define MII_SR_AUTONEG_CAPS      0x0008	/* Auto Neg Capable */
#define MII_SR_REMOTE_FAULT      0x0010	/* Remote Fault Detect */
#define MII_SR_AUTONEG_COMPLETE  0x0020	/* Auto Neg Complete */
#define MII_SR_PREAMBLE_SUPPRESS 0x0040	/* Preamble may be suppressed */
#define MII_SR_EXTENDED_STATUS   0x0100	/* Ext. status info in Reg 0x0F */
#define MII_SR_100T2_HD_CAPS     0x0200	/* 100T2 Half Duplex Capable */
#define MII_SR_100T2_FD_CAPS     0x0400	/* 100T2 Full Duplex Capable */
#define MII_SR_10T_HD_CAPS       0x0800	/* 10T   Half Duplex Capable */
#define MII_SR_10T_FD_CAPS       0x1000	/* 10T   Full Duplex Capable */
#define MII_SR_100X_HD_CAPS      0x2000	/* 100X  Half Duplex Capable */
#define MII_SR_100X_FD_CAPS      0x4000	/* 100X  Full Duplex Capable */
#define MII_SR_100T4_CAPS        0x8000	/* 100T4 Capable */

/* Autoneg Advertisement Register */
#define NWAY_AR_SELECTOR_FIELD 0x0001	/* indicates IEEE 802.3 CSMA/CD */
#define NWAY_AR_10T_HD_CAPS    0x0020	/* 10T   Half Duplex Capable */
#define NWAY_AR_10T_FD_CAPS    0x0040	/* 10T   Full Duplex Capable */
#define NWAY_AR_100TX_HD_CAPS  0x0080	/* 100TX Half Duplex Capable */
#define NWAY_AR_100TX_FD_CAPS  0x0100	/* 100TX Full Duplex Capable */
#define NWAY_AR_100T4_CAPS     0x0200	/* 100T4 Capable */
#define NWAY_AR_PAUSE          0x0400	/* Pause operation desired */
#define NWAY_AR_ASM_DIR        0x0800	/* Asymmetric Pause Direction bit */
#define NWAY_AR_REMOTE_FAULT   0x2000	/* Remote Fault detected */
#define NWAY_AR_NEXT_PAGE      0x8000	/* Next Page ability supported */

/* Link Partner Ability Register (Base Page) */
#define NWAY_LPAR_SELECTOR_FIELD 0x0000	/* LP protocol selector field */
#define NWAY_LPAR_10T_HD_CAPS    0x0020	/* LP is 10T   Half Duplex Capable */
#define NWAY_LPAR_10T_FD_CAPS    0x0040	/* LP is 10T   Full Duplex Capable */
#define NWAY_LPAR_100TX_HD_CAPS  0x0080	/* LP is 100TX Half Duplex Capable */
#define NWAY_LPAR_100TX_FD_CAPS  0x0100	/* LP is 100TX Full Duplex Capable */
#define NWAY_LPAR_100T4_CAPS     0x0200	/* LP is 100T4 Capable */
#define NWAY_LPAR_PAUSE          0x0400	/* LP Pause operation desired */
#define NWAY_LPAR_ASM_DIR        0x0800	/* LP Asymmetric Pause Direction bit */
#define NWAY_LPAR_REMOTE_FAULT   0x2000	/* LP has detected Remote Fault */
#define NWAY_LPAR_ACKNOWLEDGE    0x4000	/* LP has rx'd link code word */
#define NWAY_LPAR_NEXT_PAGE      0x8000	/* Next Page ability supported */

/* Autoneg Expansion Register */
#define NWAY_ER_LP_NWAY_CAPS      0x0001	/* LP has Auto Neg Capability */
#define NWAY_ER_PAGE_RXD          0x0002	/* LP is 10T   Half Duplex Capable */
#define NWAY_ER_NEXT_PAGE_CAPS    0x0004	/* LP is 10T   Full Duplex Capable */
#define NWAY_ER_LP_NEXT_PAGE_CAPS 0x0008	/* LP is 100TX Half Duplex Capable */
#define NWAY_ER_PAR_DETECT_FAULT  0x0010	/* LP is 100TX Full Duplex Capable */

/* Next Page TX Register */
#define NPTX_MSG_CODE_FIELD 0x0001	/* NP msg code or unformatted data */
#define NPTX_TOGGLE         0x0800	/* Toggles between exchanges
					 * of different NP
					 */
#define NPTX_ACKNOWLDGE2    0x1000	/* 1 = will comply with msg
					 * 0 = cannot comply with msg
					 */
#define NPTX_MSG_PAGE       0x2000	/* formatted(1)/unformatted(0) pg */
#define NPTX_NEXT_PAGE      0x8000	/* 1 = addition NP will follow
					 * 0 = sending last NP
					 */

/* Link Partner Next Page Register */
#define LP_RNPR_MSG_CODE_FIELD 0x0001	/* NP msg code or unformatted data */
#define LP_RNPR_TOGGLE         0x0800	/* Toggles between exchanges
					 * of different NP
					 */
#define LP_RNPR_ACKNOWLDGE2    0x1000	/* 1 = will comply with msg
					 * 0 = cannot comply with msg
					 */
#define LP_RNPR_MSG_PAGE       0x2000	/* formatted(1)/unformatted(0) pg */
#define LP_RNPR_ACKNOWLDGE     0x4000	/* 1 = ACK / 0 = NO ACK */
#define LP_RNPR_NEXT_PAGE      0x8000	/* 1 = addition NP will follow
					 * 0 = sending last NP
					 */

/* 1000BASE-T Control Register */
#define CR_1000T_ASYM_PAUSE      0x0080	/* Advertise asymmetric pause bit */
#define CR_1000T_HD_CAPS         0x0100	/* Advertise 1000T HD capability */
#define CR_1000T_FD_CAPS         0x0200	/* Advertise 1000T FD capability  */
#define CR_1000T_REPEATER_DTE    0x0400	/* 1=Repeater/switch device port */
					/* 0=DTE device */
#define CR_1000T_MS_VALUE        0x0800	/* 1=Configure PHY as Master */
					/* 0=Configure PHY as Slave */
#define CR_1000T_MS_ENABLE       0x1000	/* 1=Master/Slave manual config value */
					/* 0=Automatic Master/Slave config */
#define CR_1000T_TEST_MODE_NORMAL 0x0000	/* Normal Operation */
#define CR_1000T_TEST_MODE_1     0x2000	/* Transmit Waveform test */
#define CR_1000T_TEST_MODE_2     0x4000	/* Master Transmit Jitter test */
#define CR_1000T_TEST_MODE_3     0x6000	/* Slave Transmit Jitter test */
#define CR_1000T_TEST_MODE_4     0x8000	/* Transmitter Distortion test */

/* 1000BASE-T Status Register */
#define SR_1000T_IDLE_ERROR_CNT   0x00FF	/* Num idle errors since last read */
#define SR_1000T_ASYM_PAUSE_DIR   0x0100	/* LP asymmetric pause direction bit */
#define SR_1000T_LP_HD_CAPS       0x0400	/* LP is 1000T HD capable */
#define SR_1000T_LP_FD_CAPS       0x0800	/* LP is 1000T FD capable */
#define SR_1000T_REMOTE_RX_STATUS 0x1000	/* Remote receiver OK */
#define SR_1000T_LOCAL_RX_STATUS  0x2000	/* Local receiver OK */
#define SR_1000T_MS_CONFIG_RES    0x4000	/* 1=Local TX is Master, 0=Slave */
#define SR_1000T_MS_CONFIG_FAULT  0x8000	/* Master/Slave config fault */
#define SR_1000T_REMOTE_RX_STATUS_SHIFT          12
#define SR_1000T_LOCAL_RX_STATUS_SHIFT           13
#define SR_1000T_PHY_EXCESSIVE_IDLE_ERR_COUNT    5
#define FFE_IDLE_ERR_COUNT_TIMEOUT_20            20
#define FFE_IDLE_ERR_COUNT_TIMEOUT_100           100

/* Extended Status Register */
#define IEEE_ESR_1000T_HD_CAPS 0x1000	/* 1000T HD capable */
#define IEEE_ESR_1000T_FD_CAPS 0x2000	/* 1000T FD capable */
#define IEEE_ESR_1000X_HD_CAPS 0x4000	/* 1000X HD capable */
#define IEEE_ESR_1000X_FD_CAPS 0x8000	/* 1000X FD capable */

#define PHY_TX_POLARITY_MASK   0x0100	/* register 10h bit 8 (polarity bit) */
#define PHY_TX_NORMAL_POLARITY 0	/* register 10h bit 8 (normal polarity) */

#define AUTO_POLARITY_DISABLE  0x0010	/* register 11h bit 4 */
				      /* (0=enable, 1=disable) */

/* M88E1000 PHY Specific Control Register */
#define M88E1000_PSCR_JABBER_DISABLE    0x0001	/* 1=Jabber Function disabled */
#define M88E1000_PSCR_POLARITY_REVERSAL 0x0002	/* 1=Polarity Reversal enabled */
#define M88E1000_PSCR_SQE_TEST          0x0004	/* 1=SQE Test enabled */
#define M88E1000_PSCR_CLK125_DISABLE    0x0010	/* 1=CLK125 low,
						 * 0=CLK125 toggling
						 */
#define M88E1000_PSCR_MDI_MANUAL_MODE  0x0000	/* MDI Crossover Mode bits 6:5 */
					       /* Manual MDI configuration */
#define M88E1000_PSCR_MDIX_MANUAL_MODE 0x0020	/* Manual MDIX configuration */
#define M88E1000_PSCR_AUTO_X_1000T     0x0040	/* 1000BASE-T: Auto crossover,
						 *  100BASE-TX/10BASE-T:
						 *  MDI Mode
						 */
#define M88E1000_PSCR_AUTO_X_MODE      0x0060	/* Auto crossover enabled
						 * all speeds.
						 */
#define M88E1000_PSCR_10BT_EXT_DIST_ENABLE 0x0080
					/* 1=Enable Extended 10BASE-T distance
					 * (Lower 10BASE-T RX Threshold)
					 * 0=Normal 10BASE-T RX Threshold */
#define M88E1000_PSCR_MII_5BIT_ENABLE      0x0100
					/* 1=5-Bit interface in 100BASE-TX
					 * 0=MII interface in 100BASE-TX */
#define M88E1000_PSCR_SCRAMBLER_DISABLE    0x0200	/* 1=Scrambler disable */
#define M88E1000_PSCR_FORCE_LINK_GOOD      0x0400	/* 1=Force link good */
#define M88E1000_PSCR_ASSERT_CRS_ON_TX     0x0800	/* 1=Assert CRS on Transmit */

#define M88E1000_PSCR_POLARITY_REVERSAL_SHIFT    1
#define M88E1000_PSCR_AUTO_X_MODE_SHIFT          5
#define M88E1000_PSCR_10BT_EXT_DIST_ENABLE_SHIFT 7

/* M88E1000 PHY Specific Status Register */
#define M88E1000_PSSR_JABBER             0x0001	/* 1=Jabber */
#define M88E1000_PSSR_REV_POLARITY       0x0002	/* 1=Polarity reversed */
#define M88E1000_PSSR_DOWNSHIFT          0x0020	/* 1=Downshifted */
#define M88E1000_PSSR_MDIX               0x0040	/* 1=MDIX; 0=MDI */
#define M88E1000_PSSR_CABLE_LENGTH       0x0380	/* 0=<50M;1=50-80M;2=80-110M;
						 * 3=110-140M;4=>140M */
#define M88E1000_PSSR_LINK               0x0400	/* 1=Link up, 0=Link down */
#define M88E1000_PSSR_SPD_DPLX_RESOLVED  0x0800	/* 1=Speed & Duplex resolved */
#define M88E1000_PSSR_PAGE_RCVD          0x1000	/* 1=Page received */
#define M88E1000_PSSR_DPLX               0x2000	/* 1=Duplex 0=Half Duplex */
#define M88E1000_PSSR_SPEED              0xC000	/* Speed, bits 14:15 */
#define M88E1000_PSSR_10MBS              0x0000	/* 00=10Mbs */
#define M88E1000_PSSR_100MBS             0x4000	/* 01=100Mbs */
#define M88E1000_PSSR_1000MBS            0x8000	/* 10=1000Mbs */

#define M88E1000_PSSR_REV_POLARITY_SHIFT 1
#define M88E1000_PSSR_DOWNSHIFT_SHIFT    5
#define M88E1000_PSSR_MDIX_SHIFT         6
#define M88E1000_PSSR_CABLE_LENGTH_SHIFT 7

/* M88E1000 Extended PHY Specific Control Register */
#define M88E1000_EPSCR_FIBER_LOOPBACK 0x4000	/* 1=Fiber loopback */
#define M88E1000_EPSCR_DOWN_NO_IDLE   0x8000	/* 1=Lost lock detect enabled.
						 * Will assert lost lock and bring
						 * link down if idle not seen
						 * within 1ms in 1000BASE-T
						 */
/* Number of times we will attempt to autonegotiate before downshifting if we
 * are the master */
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_MASK 0x0C00
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_1X   0x0000
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_2X   0x0400
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_3X   0x0800
#define M88E1000_EPSCR_MASTER_DOWNSHIFT_4X   0x0C00
/* Number of times we will attempt to autonegotiate before downshifting if we
 * are the slave */
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_MASK  0x0300
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_DIS   0x0000
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_1X    0x0100
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_2X    0x0200
#define M88E1000_EPSCR_SLAVE_DOWNSHIFT_3X    0x0300
#define M88E1000_EPSCR_TX_CLK_2_5     0x0060	/* 2.5 MHz TX_CLK */
#define M88E1000_EPSCR_TX_CLK_25      0x0070	/* 25  MHz TX_CLK */
#define M88E1000_EPSCR_TX_CLK_0       0x0000	/* NO  TX_CLK */

/* M88EC018 Rev 2 specific DownShift settings */
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_MASK  0x0E00
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_1X    0x0000
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_2X    0x0200
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_3X    0x0400
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_4X    0x0600
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_5X    0x0800
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_6X    0x0A00
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_7X    0x0C00
#define M88EC018_EPSCR_DOWNSHIFT_COUNTER_8X    0x0E00

/* IGP01E1000 Specific Port Config Register - R/W */
#define IGP01E1000_PSCFR_AUTO_MDIX_PAR_DETECT  0x0010
#define IGP01E1000_PSCFR_PRE_EN                0x0020
#define IGP01E1000_PSCFR_SMART_SPEED           0x0080
#define IGP01E1000_PSCFR_DISABLE_TPLOOPBACK    0x0100
#define IGP01E1000_PSCFR_DISABLE_JABBER        0x0400
#define IGP01E1000_PSCFR_DISABLE_TRANSMIT      0x2000

/* IGP01E1000 Specific Port Status Register - R/O */
#define IGP01E1000_PSSR_AUTONEG_FAILED         0x0001	/* RO LH SC */
#define IGP01E1000_PSSR_POLARITY_REVERSED      0x0002
#define IGP01E1000_PSSR_CABLE_LENGTH           0x007C
#define IGP01E1000_PSSR_FULL_DUPLEX            0x0200
#define IGP01E1000_PSSR_LINK_UP                0x0400
#define IGP01E1000_PSSR_MDIX                   0x0800
#define IGP01E1000_PSSR_SPEED_MASK             0xC000	/* speed bits mask */
#define IGP01E1000_PSSR_SPEED_10MBPS           0x4000
#define IGP01E1000_PSSR_SPEED_100MBPS          0x8000
#define IGP01E1000_PSSR_SPEED_1000MBPS         0xC000
#define IGP01E1000_PSSR_CABLE_LENGTH_SHIFT     0x0002	/* shift right 2 */
#define IGP01E1000_PSSR_MDIX_SHIFT             0x000B	/* shift right 11 */

/* IGP01E1000 Specific Port Control Register - R/W */
#define IGP01E1000_PSCR_TP_LOOPBACK            0x0010
#define IGP01E1000_PSCR_CORRECT_NC_SCMBLR      0x0200
#define IGP01E1000_PSCR_TEN_CRS_SELECT         0x0400
#define IGP01E1000_PSCR_FLIP_CHIP              0x0800
#define IGP01E1000_PSCR_AUTO_MDIX              0x1000
#define IGP01E1000_PSCR_FORCE_MDI_MDIX         0x2000	/* 0-MDI, 1-MDIX */

/* IGP01E1000 Specific Port Link Health Register */
#define IGP01E1000_PLHR_SS_DOWNGRADE           0x8000
#define IGP01E1000_PLHR_GIG_SCRAMBLER_ERROR    0x4000
#define IGP01E1000_PLHR_MASTER_FAULT           0x2000
#define IGP01E1000_PLHR_MASTER_RESOLUTION      0x1000
#define IGP01E1000_PLHR_GIG_REM_RCVR_NOK       0x0800	/* LH */
#define IGP01E1000_PLHR_IDLE_ERROR_CNT_OFLOW   0x0400	/* LH */
#define IGP01E1000_PLHR_DATA_ERR_1             0x0200	/* LH */
#define IGP01E1000_PLHR_DATA_ERR_0             0x0100
#define IGP01E1000_PLHR_AUTONEG_FAULT          0x0040
#define IGP01E1000_PLHR_AUTONEG_ACTIVE         0x0010
#define IGP01E1000_PLHR_VALID_CHANNEL_D        0x0008
#define IGP01E1000_PLHR_VALID_CHANNEL_C        0x0004
#define IGP01E1000_PLHR_VALID_CHANNEL_B        0x0002
#define IGP01E1000_PLHR_VALID_CHANNEL_A        0x0001

/* IGP01E1000 Channel Quality Register */
#define IGP01E1000_MSE_CHANNEL_D        0x000F
#define IGP01E1000_MSE_CHANNEL_C        0x00F0
#define IGP01E1000_MSE_CHANNEL_B        0x0F00
#define IGP01E1000_MSE_CHANNEL_A        0xF000

#define IGP02E1000_PM_SPD                         0x0001	/* Smart Power Down */
#define IGP02E1000_PM_D3_LPLU                     0x0004	/* Enable LPLU in non-D0a modes */
#define IGP02E1000_PM_D0_LPLU                     0x0002	/* Enable LPLU in D0a mode */

/* IGP01E1000 DSP reset macros */
#define DSP_RESET_ENABLE     0x0
#define DSP_RESET_DISABLE    0x2
#define E1000_MAX_DSP_RESETS 10

/* IGP01E1000 & IGP02E1000 AGC Registers */

#define IGP01E1000_AGC_LENGTH_SHIFT 7	/* Coarse - 13:11, Fine - 10:7 */
#define IGP02E1000_AGC_LENGTH_SHIFT 9	/* Coarse - 15:13, Fine - 12:9 */

/* IGP02E1000 AGC Register Length 9-bit mask */
#define IGP02E1000_AGC_LENGTH_MASK  0x7F

/* 7 bits (3 Coarse + 4 Fine) --> 128 optional values */
#define IGP01E1000_AGC_LENGTH_TABLE_SIZE 128
#define IGP02E1000_AGC_LENGTH_TABLE_SIZE 113

/* The precision error of the cable length is +/- 10 meters */
#define IGP01E1000_AGC_RANGE    10
#define IGP02E1000_AGC_RANGE    15

/* IGP01E1000 PCS Initialization register */
/* bits 3:6 in the PCS registers stores the channels polarity */
#define IGP01E1000_PHY_POLARITY_MASK    0x0078

/* IGP01E1000 GMII FIFO Register */
#define IGP01E1000_GMII_FLEX_SPD               0x10	/* Enable flexible speed
							 * on Link-Up */
#define IGP01E1000_GMII_SPD                    0x20	/* Enable SPD */

/* IGP01E1000 Analog Register */
#define IGP01E1000_ANALOG_SPARE_FUSE_STATUS       0x20D1
#define IGP01E1000_ANALOG_FUSE_STATUS             0x20D0
#define IGP01E1000_ANALOG_FUSE_CONTROL            0x20DC
#define IGP01E1000_ANALOG_FUSE_BYPASS             0x20DE

#define IGP01E1000_ANALOG_FUSE_POLY_MASK            0xF000
#define IGP01E1000_ANALOG_FUSE_FINE_MASK            0x0F80
#define IGP01E1000_ANALOG_FUSE_COARSE_MASK          0x0070
#define IGP01E1000_ANALOG_SPARE_FUSE_ENABLED        0x0100
#define IGP01E1000_ANALOG_FUSE_ENABLE_SW_CONTROL    0x0002

#define IGP01E1000_ANALOG_FUSE_COARSE_THRESH        0x0040
#define IGP01E1000_ANALOG_FUSE_COARSE_10            0x0010
#define IGP01E1000_ANALOG_FUSE_FINE_1               0x0080
#define IGP01E1000_ANALOG_FUSE_FINE_10              0x0500

/* 3085~3110 */
/* Miscellaneous PHY bit definitions. */
#define PHY_PREAMBLE        0xFFFFFFFF
#define PHY_SOF             0x01
#define PHY_OP_READ         0x02
#define PHY_OP_WRITE        0x01
#define PHY_TURNAROUND      0x02
#define PHY_PREAMBLE_SIZE   32
#define MII_CR_SPEED_1000   0x0040
#define MII_CR_SPEED_100    0x2000
#define MII_CR_SPEED_10     0x0000
#define E1000_PHY_ADDRESS   0x01
#define PHY_AUTO_NEG_TIME   45	/* 4.5 Seconds */
#define PHY_FORCE_TIME      20	/* 2.0 Seconds */
#define PHY_REVISION_MASK   0xFFFFFFF0
#define DEVICE_SPEED_MASK   0x00000300	/* Device Ctrl Reg Speed Mask */
#define REG4_SPEED_MASK     0x01E0
#define REG9_SPEED_MASK     0x0300
#define ADVERTISE_10_HALF   0x0001
#define ADVERTISE_10_FULL   0x0002
#define ADVERTISE_100_HALF  0x0004
#define ADVERTISE_100_FULL  0x0008
#define ADVERTISE_1000_HALF 0x0010
#define ADVERTISE_1000_FULL 0x0020
#define AUTONEG_ADVERTISE_SPEED_DEFAULT 0x002F	/* Everything but 1000-Half */
#define AUTONEG_ADVERTISE_10_100_ALL    0x000F	/* All 10/100 speeds */
#define AUTONEG_ADVERTISE_10_ALL        0x0003	/* 10Mbps Full & Half speeds */

/* 2319~2328 */
/* Collision related configuration parameters */
#define E1000_COLLISION_THRESHOLD       15
#define E1000_CT_SHIFT                  4
/* Collision distance is a 0-based value that applies to
 * half-duplex-capable hardware only. */
#define E1000_COLLISION_DISTANCE        63
#define E1000_COLLISION_DISTANCE_82542  64
#define E1000_FDX_COLLISION_DISTANCE    E1000_COLLISION_DISTANCE
#define E1000_HDX_COLLISION_DISTANCE    E1000_COLLISION_DISTANCE
#define E1000_COLD_SHIFT                12


/* receive control register */
#define	E1000REG_RCTL				(0x100)
#define E1000REG_RCTL_RST          	0x00000001		/* Software reset */
#define	E1000REG_RCTL_EN			(0x1 << 1)		/* receiver enable */
#define E1000REG_RCTL_SBP			(0x1 << 2)
#define E1000REG_RCTL_UPE			(0x1 << 3)
#define	E1000REG_RCTL_LPE			(0x1 << 5)		/* long packet enable */
#define	E1000REG_RCTL_BAM			(0x1 << 15)		/* broadcast accept mode */
#define E1000REG_RCTL_SECRC			(0x1 << 26)		/* Strip Ethernet CRC */


#define E1000REG_FCTTV    0x00170	/* Flow Control Transmit Timer Value - RW */

/* transmit control register */
#define	E1000REG_TCTL				(0x400)
/* Transmit Control */
#define E1000_TCTL_RST    0x00000001	/* software reset */
#define E1000_TCTL_EN     0x00000002	/* enable tx */
#define E1000_TCTL_BCE    0x00000004	/* busy check enable */
#define E1000_TCTL_PSP    0x00000008	/* pad short packets */
#define E1000_TCTL_CT     0x00000ff0	/* collision threshold */
#define E1000_TCTL_COLD   0x003ff000	/* collision distance */
#define E1000_TCTL_SWXOFF 0x00400000	/* SW Xoff transmission */
#define E1000_TCTL_PBE    0x00800000	/* Packet Burst Enable */
#define E1000_TCTL_RTLC   0x01000000	/* Re-transmit on late collision */
#define E1000_TCTL_NRTU   0x02000000	/* No Re-transmit on underrun */
#define E1000_TCTL_MULR   0x10000000	/* Multiple request support */


/* some receive descriptor registers */
#define E1000REG_RDBAL				(0x2800)
#define E1000REG_RDBAH				(0x2804)
#define E1000REG_RDLEN				(0x2808)
#define E1000REG_RDH				(0x2810)
#define E1000REG_RDT				(0x2818)

/* some transmit descriptor registers */
#define E1000REG_TDBAL				(0x3800)
#define E1000REG_TDBAH				(0x3804)
#define E1000REG_TDLEN				(0x3808)
#define	E1000REG_TDH				(0x3810)
#define	E1000REG_TDT				(0x3818)

#define E1000REG_TXDCTL				0x03828	/* TX Descriptor Control - RW */
/* Transmit Descriptor Control */
#define E1000_TXDCTL_PTHRESH 0x0000003F	/* TXDCTL Prefetch Threshold */
#define E1000_TXDCTL_HTHRESH 0x00003F00	/* TXDCTL Host Threshold */
#define E1000_TXDCTL_WTHRESH 0x003F0000	/* TXDCTL Writeback Threshold */
#define E1000_TXDCTL_GRAN    0x01000000	/* TXDCTL Granularity */
#define E1000_TXDCTL_LWTHRESH 0xFE000000	/* TXDCTL Low Threshold */
#define E1000_TXDCTL_FULL_TX_DESC_WB 0x01010000	/* GRAN=1, WTHRESH=1 */
#define E1000_TXDCTL_COUNT_DESC 0x00400000	/* Enable the counting of desc.
						   still to be processed. */

#define E1000REG_MTA(i)				(0x5200 + (i) * 4)

#define E1000REG_N_RA				(16)
#define	E1000REG_RAL_BASS			(0x5400)
#define	E1000REG_RAH_BASS			(0x5404)
#define	E1000REG_RAL(n)				(E1000REG_RAL_BASS + ((n) << 3))
#define	E1000REG_RAH(n)				(E1000REG_RAH_BASS + ((n) << 3))

#define E1000REG_VFTA				0x05600	/* VLAN Filter Table Array - RW Array */

#define E1000REG_WUC				0x5800	/* Wakeup Control - RW */

#define E1000REG_MANC				0x05820	/* Management Control - RW */
/* Management Control */
#define E1000_MANC_SMBUS_EN      0x00000001	/* SMBus Enabled - RO */
#define E1000_MANC_ASF_EN        0x00000002	/* ASF Enabled - RO */
#define E1000_MANC_R_ON_FORCE    0x00000004	/* Reset on Force TCO - RO */
#define E1000_MANC_RMCP_EN       0x00000100	/* Enable RCMP 026Fh Filtering */
#define E1000_MANC_0298_EN       0x00000200	/* Enable RCMP 0298h Filtering */
#define E1000_MANC_IPV4_EN       0x00000400	/* Enable IPv4 */
#define E1000_MANC_IPV6_EN       0x00000800	/* Enable IPv6 */
#define E1000_MANC_SNAP_EN       0x00001000	/* Accept LLC/SNAP */
#define E1000_MANC_ARP_EN        0x00002000	/* Enable ARP Request Filtering */
#define E1000_MANC_NEIGHBOR_EN   0x00004000	/* Enable Neighbor Discovery
						 * Filtering */

typedef struct e1000_rxdesc {
	u64		buffaddr;
 	u16		length;		// Length of data DMAed into data buffer
	u16		csum;		// Packet checksum
	u8		status;		// Descriptor status
	u8		errors;		// Descriptor Errors
	u16		special;
} e1000_rxdesc_t;

typedef struct e1000_txdesc {
	u64		buffaddr;
	union {
		u32		data;
		struct {
			u16		length;		// Data buffer length
			u8		cso;		// Checksum offset
			u8		cmd;		// Descriptor control
		} flags;
	} lower;
	union {
		u32		data;
		struct {
			u8		status;		// Descriptor status
			u8		css;		// Checksum start
			u16		special;
		} fields;
	} upper;
} e1000_txdesc_t;

static struct pci_dev *dbg_e1000;

static void e1000_writeR(struct pci_dev *dev, u32 off, u32 value)
{
	*((volatile u32 *)(dev->bar_mem[0] + off)) = value;
}

static u32 e1000_readR(struct pci_dev *dev, u32 off)
{
	volatile u32 ret;

	ret = *((u32 *)(dev->bar_mem[0] + off));

	return ret;
}

/*
 * 'IOADDR' register: io_bar + 0
 * 'IODATA' register: io_bar + 4
 * config the 'IOADDR' with dest register offset value (param 'off')
 * read (write) the value from(into) 'IODATA'
 */

static void e1000_writeR_IO(struct pci_dev *dev, u32 off, u32 value)
{
	outl(off, dev->bar_io);
	outl(value, dev->bar_io + 4);
}


static u32 e1000_readR_IO(struct pci_dev *dev, u32 off)
{
	outl(off, dev->bar_io);
	return inl(dev->bar_io + 4);
}


static int e1000_read_phy_reg(struct pci_dev *dev, u32 addr, u16 *v)
{
	u32 i, mdic = ((addr << E1000_MDIC_REG_SHIFT) |
		(1 << E1000_MDIC_PHY_SHIFT) |
		(E1000_MDIC_OP_READ));

	e1000_writeR(dev, E1000REG_MDIC, mdic);

	/* Poll the ready bit to see if the MDI read
	 * completed
	 */
	for (i = 0; i < 64; i++) {
		wait_ms(1);
		mdic = e1000_readR(dev, E1000REG_MDIC);
		if (mdic & E1000_MDIC_READY)
			break;
	}
	if (!(mdic & E1000_MDIC_READY)) {
		printk("MDI Read did not complete\n");
		return -1;
	}
	if (mdic & E1000_MDIC_ERROR) {
		printk("MDI Error\n");
		return -1;
	}

	*v = (u16) mdic;

	return 0;
}

static int e1000_write_phy_reg(struct pci_dev *dev, u32 addr, u16 v)
{
	u32 i, mdic = (((u32) v) |
		(addr << E1000_MDIC_REG_SHIFT) |
		(1 << E1000_MDIC_PHY_SHIFT) |
		(E1000_MDIC_OP_WRITE));

	e1000_writeR(dev, E1000REG_MDIC, mdic);

	/* Poll the ready bit to see if the MDI read
	 * completed
	 */
	for (i = 0; i < 641; i++) {
		wait_ms(1);
		mdic = e1000_readR(dev, E1000REG_MDIC);
		if (mdic & E1000_MDIC_READY)
			break;
	}
	if (!(mdic & E1000_MDIC_READY)) {
		printk("MDI Write did not complete\n");
		return -1;
	}

	return 0;
}
static u32 e1000_intstat[16] = {0};

#if 0
static void testsub_dispcpu()
{
	u32 cur_cs, cur_ds, cur_es, cur_ss, cur_esp;
	__asm__ __volatile__ (
		"movl	%%cs, %%eax			\n\t"
		"movl	%%eax, %0			\n\t"

		"movl	%%ds, %%eax			\n\t"
		"movl	%%eax, %1			\n\t"
		"movl	%%es, %%eax			\n\t"
		"movl	%%eax, %2			\n\t"

		"movl	%%ss, %%eax			\n\t"
		"movl	%%eax, %3			\n\t"

		"movl	%%esp, %4			\n\t"
		:"=m"(cur_cs), "=m"(cur_ds), "=m"(cur_es),
		 "=m"(cur_ss), "=m"(cur_esp)
		:
		:"%eax"
	);

	printk("current regs:\n"
		   "cs: 0x%#4x, ds: 0x%#4x, es: 0x%#4x, ss: 0x%#4x\n"
		   "esp: 0x%#8x\n"
		   "CR0: 0x%#8x, CR3: 0x%#8x\n",
		   cur_cs, cur_ds, cur_es, cur_ss,
		   cur_esp,
		   getCR0(), getCR3());
}
#endif

static void intele1000_isr(struct pt_regs *regs, void *param)
{
#if 0
	testsub_dispcpu();
#endif

	struct pci_dev *dev = (struct pci_dev *)param;
	netdev_t *e1000 = (netdev_t *)dev->param;
	e1000_rxdesc_t *rdbuff = (e1000_rxdesc_t *)(e1000->rxdexc_ring);
	u32 icr = e1000_readR(dev, E1000REG_ICR);
{
    u32 count, flag = 0x1;
    for (count = 0; count < 16; count++)
    {
        if (flag & icr)
        {
            e1000_intstat[count]++;
        }
        flag <<= 1;
    }
}

	// printk("icr:0x%#8x.\n", icr);
	if (E1000INTFLAG_RXT0 & icr)
	{
		u32 rdh = e1000_readR(dev, E1000REG_RDH);
		u32	rdt = e1000_readR(dev, E1000REG_RDT);
		while (((rdt + 1) % (e1000->n_rxdesc)) != rdh)
		{
			/* receive a packet and insert in to the eh_proc */
	        ethframe_t *new_ef;

			rdt = (rdt + 1) % (e1000->n_rxdesc);

            new_ef = (ethframe_t *)page_alloc(BUDDY_RANK_4K, MMAREA_NORMAL);
            new_ef->netdev = e1000;
            new_ef->len = rdbuff[rdt].length;
            new_ef->buf = new_ef->bufs + 128;		/* reserve 128 Byte, used to packet head modify */
            memcpy(new_ef->buf, (void *)((u32)(rdbuff[rdt].buffaddr)), new_ef->len);

            rxef_insert(new_ef);
		}

		e1000_writeR(dev, E1000REG_RDT, rdt);
	}
}

static int intele1000_tx(netdev_t *netdev, void *buf, u32 len)
{
	struct pci_dev *dev = netdev->dev;
	e1000_txdesc_t *txbuff = (e1000_txdesc_t *)(netdev->txdexc_ring);

	u32 tdh = e1000_readR(dev, E1000REG_TDH);
	u32	tdt_op, tdt = e1000_readR(dev, E1000REG_TDT);

	tdt_op = tdt;
	tdt = (tdt + 1) % (netdev->n_txdesc);
	if (tdt == tdh)
	{
		printk("intele1000_tx() faild.\n");
		/* no enough transmit descriptor */
		return -1;
	}

	memcpy((void *)(u32)(txbuff[tdt_op].buffaddr), buf, len);
	txbuff[tdt_op].lower.flags.length = (u16)len;
	txbuff[tdt_op].lower.flags.cmd = 3;

	e1000_writeR(dev, E1000REG_TDT, tdt);

	return 0;
}

/* init receive descriptor */
static void intele1000_initrxdsc(netdev_t *e1000, u32 descbuff, u16 n_desc)
{
	u32 count;
	u32 rbbase = (u32)(e1000->rxDMA);
	ASSERT(sizeof(e1000_rxdesc_t) == 16);
	e1000_rxdesc_t *rd = (e1000_rxdesc_t *)descbuff;
	for (count = 0; count < n_desc; count++)
	{
		rd->buffaddr = rbbase;

		rbbase += e1000->rd_bufsize;
		rd++;
	}
}

/* init transmit descriptor */
static void intele1000_inittxdsc(netdev_t *e1000, u32 descbuff, u16 n_desc)
{
	u32 count;
	u32 rbbase = (u32)(e1000->txDMA);
	ASSERT(sizeof(e1000_txdesc_t) == 16);
	e1000_txdesc_t *td = (e1000_txdesc_t *)descbuff;
	for (count = 0; count < n_desc; count++)
	{
		td->buffaddr = rbbase;

		rbbase += e1000->td_bufsize;
		td++;
	}
}

static void e1000_copper_link_preconfig(struct pci_dev *dev)
{

	u32 ctrl;
#if 0
	s32 ret_val;
	u16 phy_data;

	e_dbg("e1000_copper_link_preconfig");

	ctrl = er32(CTRL);
	/* With 82543, we need to force speed and duplex on the MAC equal to
	 * what the PHY speed and duplex configuration is. In addition, we need
	 * to perform a hardware reset on the PHY to take it out of reset.
	 */
	if (hw->mac_type > e1000_82543) {
#endif
	ctrl = e1000_readR(dev, E1000REG_CTRL);
	ctrl |= E1000REG_CTRL_SLU;
	ctrl &= ~(E1000REG_CTRL_FRCSPD | E1000REG_CTRL_FRCDPLX);
	e1000_writeR(dev, E1000REG_CTRL, ctrl);
#if 0
	} else {
		ctrl |=
		    (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX | E1000_CTRL_SLU);
		ew32(CTRL, ctrl);
		ret_val = e1000_phy_hw_reset(hw);
		if (ret_val)
			return ret_val;
	}

	/* Make sure we have a valid PHY */
	ret_val = e1000_detect_gig_phy(hw);
	if (ret_val) {
		e_dbg("Error, did not detect valid phy.\n");
		return ret_val;
	}
	e_dbg("Phy ID = %x\n", hw->phy_id);

	/* Set PHY to class A mode (if necessary) */
	ret_val = e1000_set_phy_mode(hw);
	if (ret_val)
		return ret_val;

	if ((hw->mac_type == e1000_82545_rev_3) ||
	    (hw->mac_type == e1000_82546_rev_3)) {
		ret_val =
		    e1000_read_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, &phy_data);
		phy_data |= 0x00000008;
		ret_val =
		    e1000_write_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, phy_data);
	}

	if (hw->mac_type <= e1000_82543 ||
	    hw->mac_type == e1000_82541 || hw->mac_type == e1000_82547 ||
	    hw->mac_type == e1000_82541_rev_2
	    || hw->mac_type == e1000_82547_rev_2)
		hw->phy_reset_disable = false;

	return E1000_SUCCESS;
#endif
}

/**
 * e1000_phy_reset - reset the phy to commit settings
 * @hw: Struct containing variables accessed by shared code
 *
 * Resets the PHY
 * Sets bit 15 of the MII Control register
 */
s32 e1000_phy_reset(struct pci_dev *dev)
{
	s32 ret_val;
	u16 phy_data = 0;

#if 0
	e_dbg("e1000_phy_reset");

	switch (hw->phy_type) {
	case e1000_phy_igp:
		ret_val = e1000_phy_hw_reset(hw);
		if (ret_val)
			return ret_val;
		break;
	default:
#endif
	ret_val = e1000_read_phy_reg(dev, PHY_CTRL, &phy_data);

	phy_data |= MII_CR_RESET;
	ret_val = e1000_write_phy_reg(dev, PHY_CTRL, phy_data);

#if 0
		udelay(1);
		break;
	}

	if (hw->phy_type == e1000_phy_igp)
		e1000_phy_init_script(hw);
#endif

	return ret_val;
}


/**
 * e1000_copper_link_mgp_setup - Copper link setup for e1000_phy_m88 series.
 * @hw: Struct containing variables accessed by shared code
 */
static void e1000_copper_link_mgp_setup(struct pci_dev *dev)
{
	u16 phy_data = 0;
#if 0
	e_dbg("e1000_copper_link_mgp_setup");

	if (hw->phy_reset_disable)
		return E1000_SUCCESS;
#endif
	/* Enable CRS on TX. This must be set for half-duplex operation. */
	e1000_read_phy_reg(dev, M88E1000_PHY_SPEC_CTRL, &phy_data);

	phy_data |= M88E1000_PSCR_ASSERT_CRS_ON_TX;

	/* Options:
	 *   MDI/MDI-X = 0 (default)
	 *   0 - Auto for all speeds
	 *   1 - MDI mode
	 *   2 - MDI-X mode
	 *   3 - Auto for 1000Base-T only (MDI-X for 10/100Base-T modes)
	 */
	phy_data &= ~M88E1000_PSCR_AUTO_X_MODE;


	phy_data |= M88E1000_PSCR_AUTO_X_MODE;


	/* Options:
	 *   disable_polarity_correction = 0 (default)
	 *       Automatic Correction for Reversed Cable Polarity
	 *   0 - Disabled
	 *   1 - Enabled
	 */
	phy_data &= ~M88E1000_PSCR_POLARITY_REVERSAL;
	e1000_write_phy_reg(dev, M88E1000_PHY_SPEC_CTRL, phy_data);

#if 0
	if (hw->phy_revision < M88E1011_I_REV_4) {
#endif
		/* Force TX_CLK in the Extended PHY Specific Control Register
		 * to 25MHz clock.
		 */
		    e1000_read_phy_reg(dev, M88E1000_EXT_PHY_SPEC_CTRL,
				       &phy_data);

		phy_data |= M88E1000_EPSCR_TX_CLK_25;

#if 0
		if ((hw->phy_revision == E1000_REVISION_2) &&
		    (hw->phy_id == M88E1111_I_PHY_ID)) {
			/* Vidalia Phy, set the downshift counter to 5x */
			phy_data &= ~(M88EC018_EPSCR_DOWNSHIFT_COUNTER_MASK);
			phy_data |= M88EC018_EPSCR_DOWNSHIFT_COUNTER_5X;
			ret_val = e1000_write_phy_reg(hw,
						      M88E1000_EXT_PHY_SPEC_CTRL,
						      phy_data);
			if (ret_val)
				return ret_val;
		} else {
#endif
			/* Configure Master and Slave downshift values */
			phy_data &= ~(M88E1000_EPSCR_MASTER_DOWNSHIFT_MASK |
				      M88E1000_EPSCR_SLAVE_DOWNSHIFT_MASK);
			phy_data |= (M88E1000_EPSCR_MASTER_DOWNSHIFT_1X |
				     M88E1000_EPSCR_SLAVE_DOWNSHIFT_1X);
			e1000_write_phy_reg(dev,
						      M88E1000_EXT_PHY_SPEC_CTRL,
						      phy_data);
#if 0
		}
	}
#endif

	/* SW Reset the PHY so all changes take effect */
	e1000_phy_reset(dev);
}

/**
 * e1000_config_collision_dist - set collision distance register
 * @hw: Struct containing variables accessed by shared code
 *
 * Sets the collision distance in the Transmit Control register.
 * Link should have been established previously. Reads the speed and duplex
 * information from the Device Status register.
 */
void e1000_config_collision_dist(struct pci_dev *dev)
{
	u32 tctl, coll_dist;

#if 0
	e_dbg("e1000_config_collision_dist");

	if (hw->mac_type < e1000_82543)
		coll_dist = E1000_COLLISION_DISTANCE_82542;
	else
#endif
		coll_dist = E1000_COLLISION_DISTANCE;

	tctl = e1000_readR(dev, E1000REG_TCTL);

	tctl &= ~E1000_TCTL_COLD;
	tctl |= coll_dist << E1000_COLD_SHIFT;

	e1000_writeR(dev, E1000REG_TCTL, tctl);
}

/**
 * e1000_copper_link_postconfig - post link setup
 * @hw: Struct containing variables accessed by shared code
 *
 * Config the MAC and the PHY after link is up.
 *   1) Set up the MAC to the current PHY speed/duplex
 *      if we are on 82543.  If we
 *      are on newer silicon, we only need to configure
 *      collision distance in the Transmit Control Register.
 *   2) Set up flow control on the MAC to that established with
 *      the link partner.
 *   3) Config DSP to improve Gigabit link quality for some PHY revisions.
 */
static s32 e1000_copper_link_postconfig(struct pci_dev *dev)
{
#if 0
	s32 ret_val;
	e_dbg("e1000_copper_link_postconfig");

	if ((hw->mac_type >= e1000_82544) && (hw->mac_type != e1000_ce4100)) {
#endif
		e1000_config_collision_dist(dev);
#if 0
	} else {
		ret_val = e1000_config_mac_to_phy(hw);
		if (ret_val) {
			e_dbg("Error configuring MAC to PHY settings\n");
			return ret_val;
		}
	}
	ret_val = e1000_config_fc_after_link_up(hw);
	if (ret_val) {
		e_dbg("Error Configuring Flow Control\n");
		return ret_val;
	}

	/* Config DSP to improve Giga link quality */
	if (hw->phy_type == e1000_phy_igp) {
		ret_val = e1000_config_dsp_after_link_change(hw, true);
		if (ret_val) {
			e_dbg("Error Configuring DSP after link up\n");
			return ret_val;
		}
	}
#endif
	return 0;
}

/**
 * e1000_phy_setup_autoneg - phy settings
 * @hw: Struct containing variables accessed by shared code
 *
 * Configures PHY autoneg and flow control advertisement settings
 */
s32 e1000_phy_setup_autoneg(struct pci_dev *dev)
{
	s32 ret_val;
	u16 mii_autoneg_adv_reg = 0;
	u16 mii_1000t_ctrl_reg = 0;

	printk("e1000_phy_setup_autoneg\n");

	/* Read the MII Auto-Neg Advertisement Register (Address 4). */
	ret_val = e1000_read_phy_reg(dev, PHY_AUTONEG_ADV, &mii_autoneg_adv_reg);
	if (ret_val)
		return ret_val;

	/* Read the MII 1000Base-T Control Register (Address 9). */
	ret_val = e1000_read_phy_reg(dev, PHY_1000T_CTRL, &mii_1000t_ctrl_reg);
	if (ret_val)
		return ret_val;

	/* Need to parse both autoneg_advertised and fc and set up
	 * the appropriate PHY registers.  First we will parse for
	 * autoneg_advertised software override.  Since we can advertise
	 * a plethora of combinations, we need to check each bit
	 * individually.
	 */

	/* First we clear all the 10/100 mb speed bits in the Auto-Neg
	 * Advertisement Register (Address 4) and the 1000 mb speed bits in
	 * the  1000Base-T Control Register (Address 9).
	 */
	mii_autoneg_adv_reg &= ~REG4_SPEED_MASK;
	mii_1000t_ctrl_reg &= ~REG9_SPEED_MASK;

#if 0
	/* Do we want to advertise 10 Mb Half Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_10_HALF) {
		e_dbg("Advertise 10mb Half duplex\n");
		mii_autoneg_adv_reg |= NWAY_AR_10T_HD_CAPS;
	}

	/* Do we want to advertise 10 Mb Full Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_10_FULL) {
		e_dbg("Advertise 10mb Full duplex\n");
		mii_autoneg_adv_reg |= NWAY_AR_10T_FD_CAPS;
	}

	/* Do we want to advertise 100 Mb Half Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_100_HALF) {
		e_dbg("Advertise 100mb Half duplex\n");
		mii_autoneg_adv_reg |= NWAY_AR_100TX_HD_CAPS;
	}

	/* Do we want to advertise 100 Mb Full Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_100_FULL) {
		e_dbg("Advertise 100mb Full duplex\n");
		mii_autoneg_adv_reg |= NWAY_AR_100TX_FD_CAPS;
	}

	/* We do not allow the Phy to advertise 1000 Mb Half Duplex */
	if (hw->autoneg_advertised & ADVERTISE_1000_HALF) {
		e_dbg
		    ("Advertise 1000mb Half duplex requested, request denied!\n");
	}
#endif
	mii_autoneg_adv_reg |= NWAY_AR_10T_HD_CAPS;
	mii_autoneg_adv_reg |= NWAY_AR_10T_FD_CAPS;
	mii_autoneg_adv_reg |= NWAY_AR_100TX_HD_CAPS;
	mii_autoneg_adv_reg |= NWAY_AR_100TX_FD_CAPS;


	/* Do we want to advertise 1000 Mb Full Duplex? */
#if 0
	if (hw->autoneg_advertised & ADVERTISE_1000_FULL) {
		e_dbg("Advertise 1000mb Full duplex\n");
		mii_1000t_ctrl_reg |= CR_1000T_FD_CAPS;
	}
#else
	mii_1000t_ctrl_reg |= CR_1000T_FD_CAPS;
#endif

	/* Check for a software override of the flow control settings, and
	 * setup the PHY advertisement registers accordingly.  If
	 * auto-negotiation is enabled, then software will have to set the
	 * "PAUSE" bits to the correct value in the Auto-Negotiation
	 * Advertisement Register (PHY_AUTONEG_ADV) and re-start
	 * auto-negotiation.
	 *
	 * The possible values of the "fc" parameter are:
	 *      0:  Flow control is completely disabled
	 *      1:  Rx flow control is enabled (we can receive pause frames
	 *          but not send pause frames).
	 *      2:  Tx flow control is enabled (we can send pause frames
	 *          but we do not support receiving pause frames).
	 *      3:  Both Rx and TX flow control (symmetric) are enabled.
	 *  other:  No software override.  The flow control configuration
	 *          in the EEPROM is used.
	 */
#if 0
	switch (hw->fc) {
	case E1000_FC_NONE:	/* 0 */
		/* Flow control (RX & TX) is completely disabled by a
		 * software over-ride.
		 */
#endif
		mii_autoneg_adv_reg &= ~(NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
#if 0
		break;
	case E1000_FC_RX_PAUSE:	/* 1 */
		/* RX Flow control is enabled, and TX Flow control is
		 * disabled, by a software over-ride.
		 */
		/* Since there really isn't a way to advertise that we are
		 * capable of RX Pause ONLY, we will advertise that we
		 * support both symmetric and asymmetric RX PAUSE.  Later
		 * (in e1000_config_fc_after_link_up) we will disable the
		 * hw's ability to send PAUSE frames.
		 */
		mii_autoneg_adv_reg |= (NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
		break;
	case E1000_FC_TX_PAUSE:	/* 2 */
		/* TX Flow control is enabled, and RX Flow control is
		 * disabled, by a software over-ride.
		 */
		mii_autoneg_adv_reg |= NWAY_AR_ASM_DIR;
		mii_autoneg_adv_reg &= ~NWAY_AR_PAUSE;
		break;
	case E1000_FC_FULL:	/* 3 */
		/* Flow control (both RX and TX) is enabled by a software
		 * over-ride.
		 */
		mii_autoneg_adv_reg |= (NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
		break;
	default:
		e_dbg("Flow control param set incorrectly\n");
		return -E1000_ERR_CONFIG;
	}
#endif

	ret_val = e1000_write_phy_reg(dev, PHY_AUTONEG_ADV, mii_autoneg_adv_reg);
	if (ret_val)
		return ret_val;

	printk("Auto-Neg Advertising 0x%4x\n", mii_autoneg_adv_reg);

#if 0
	if (hw->phy_type == e1000_phy_8201) {
		mii_1000t_ctrl_reg = 0;
	} else {
#endif
		ret_val = e1000_write_phy_reg(dev, PHY_1000T_CTRL,
		                              mii_1000t_ctrl_reg);
#if 0
		if (ret_val)
			return ret_val;
	}
#endif
	return 0;
}

/**
 * e1000_copper_link_autoneg - setup auto-neg
 * @hw: Struct containing variables accessed by shared code
 *
 * Setup auto-negotiation and flow control advertisements,
 * and then perform auto-negotiation.
 */
static s32 e1000_copper_link_autoneg(struct pci_dev *dev)
{
	s32 ret_val;
	u16 phy_data = 0;
#if 0

	e_dbg("e1000_copper_link_autoneg");

	/* Perform some bounds checking on the hw->autoneg_advertised
	 * parameter.  If this variable is zero, then set it to the default.
	 */
	hw->autoneg_advertised &= AUTONEG_ADVERTISE_SPEED_DEFAULT;

	/* If autoneg_advertised is zero, we assume it was not defaulted
	 * by the calling code so we set to advertise full capability.
	 */
	if (hw->autoneg_advertised == 0)
		hw->autoneg_advertised = AUTONEG_ADVERTISE_SPEED_DEFAULT;

	/* IFE/RTL8201N PHY only supports 10/100 */
	if (hw->phy_type == e1000_phy_8201)
		hw->autoneg_advertised &= AUTONEG_ADVERTISE_10_100_ALL;

	e_dbg("Reconfiguring auto-neg advertisement params\n");
#endif
	ret_val = e1000_phy_setup_autoneg(dev);
#if 0
	if (ret_val) {
		e_dbg("Error Setting up Auto-Negotiation\n");
		return ret_val;
	}
#endif
	printk("Restarting Auto-Neg\n");

	/* Restart auto-negotiation by setting the Auto Neg Enable bit and
	 * the Auto Neg Restart bit in the PHY control register.
	 */
	ret_val = e1000_read_phy_reg(dev, PHY_CTRL, &phy_data);
	if (ret_val)
		return ret_val;

	phy_data |= (MII_CR_AUTO_NEG_EN | MII_CR_RESTART_AUTO_NEG);
	ret_val = e1000_write_phy_reg(dev, PHY_CTRL, phy_data);
	if (ret_val)
		return ret_val;

#if 0
	/* Does the user want to wait for Auto-Neg to complete here, or
	 * check at a later time (for example, callback routine).
	 */
	if (hw->wait_autoneg_complete) {
		ret_val = e1000_wait_autoneg(hw);
		if (ret_val) {
			e_dbg
			    ("Error while waiting for autoneg to complete\n");
			return ret_val;
		}
	}

	hw->get_link_status = true;
#endif
	return 0;
}

static int e1000_setup_copper_link(struct pci_dev *dev)
{
	s32 ret_val;
	u16 i;
	u16 phy_data = 0;
#if 0
	e_dbg("e1000_setup_copper_link");
#endif
	/* Check if it is a valid PHY and set PHY mode if necessary. */
#if 0
	ret_val = e1000_copper_link_preconfig(hw);
	if (ret_val)
		return ret_val;
#else
	e1000_copper_link_preconfig(dev);
#endif

#if 0
	if (hw->phy_type == e1000_phy_igp) {
		ret_val = e1000_copper_link_igp_setup(hw);
		if (ret_val)
			return ret_val;
	} else if (hw->phy_type == e1000_phy_m88) {
#endif
#if 0
		ret_val = e1000_copper_link_mgp_setup(hw);
		if (ret_val)
			return ret_val;
#else
	e1000_copper_link_mgp_setup(dev);
#endif
#if 0
	} else {
		ret_val = gbe_dhg_phy_setup(hw);
		if (ret_val) {
			e_dbg("gbe_dhg_phy_setup failed!\n");
			return ret_val;
		}
	}
#endif

#if 0
	if (hw->autoneg) {
#endif
		/* Setup autoneg and flow control advertisement
		 * and perform autonegotiation
		 */
		ret_val = e1000_copper_link_autoneg(dev);
#if 0
		if (ret_val)
			return ret_val;
	} else {
		/* PHY will be set to 10H, 10F, 100H,or 100F
		 * depending on value from forced_speed_duplex.
		 */
		e_dbg("Forcing speed and duplex\n");
		ret_val = e1000_phy_force_speed_duplex(hw);
		if (ret_val) {
			e_dbg("Error Forcing Speed and Duplex\n");
			return ret_val;
		}
	}
#endif

	/* Check link status. Wait up to 100 microseconds for link to become
	 * valid.
	 */
	for (i = 0; i < 10; i++) {
		ret_val = e1000_read_phy_reg(dev, PHY_STATUS, &phy_data);
		if (ret_val)
			return ret_val;
		ret_val = e1000_read_phy_reg(dev, PHY_STATUS, &phy_data);
		if (ret_val)
			return ret_val;

		if (phy_data & MII_SR_LINK_STATUS) {
			/* Config the MAC and PHY after link is up */
			ret_val = e1000_copper_link_postconfig(dev);
			if (ret_val)
				return ret_val;

			printk("Valid link established!!!\n");
			return 0;
		}
		wait_ms(1);
	}

	printk("Unable to establish link!!!\n");
	return 0;
}

void e1000_setup_link(struct pci_dev *dev)
{
#if 0
	u32 ctrl_ext;
	s32 ret_val;
	u16 eeprom_data;

	e_dbg("e1000_setup_link");

	/* Read and store word 0x0F of the EEPROM. This word contains bits
	 * that determine the hardware's default PAUSE (flow control) mode,
	 * a bit that determines whether the HW defaults to enabling or
	 * disabling auto-negotiation, and the direction of the
	 * SW defined pins. If there is no SW over-ride of the flow
	 * control setting, then the variable hw->fc will
	 * be initialized based on a value in the EEPROM.
	 */
	if (hw->fc == E1000_FC_DEFAULT) {
		ret_val = e1000_read_eeprom(hw, EEPROM_INIT_CONTROL2_REG,
					    1, &eeprom_data);
		if (ret_val) {
			e_dbg("EEPROM Read Error\n");
			return -E1000_ERR_EEPROM;
		}
		if ((eeprom_data & EEPROM_WORD0F_PAUSE_MASK) == 0)
			hw->fc = E1000_FC_NONE;
		else if ((eeprom_data & EEPROM_WORD0F_PAUSE_MASK) ==
			 EEPROM_WORD0F_ASM_DIR)
			hw->fc = E1000_FC_TX_PAUSE;
		else
			hw->fc = E1000_FC_FULL;
	}

	/* We want to save off the original Flow Control configuration just
	 * in case we get disconnected and then reconnected into a different
	 * hub or switch with different Flow Control capabilities.
	 */
	if (hw->mac_type == e1000_82542_rev2_0)
		hw->fc &= (~E1000_FC_TX_PAUSE);

	if ((hw->mac_type < e1000_82543) && (hw->report_tx_early == 1))
		hw->fc &= (~E1000_FC_RX_PAUSE);

	hw->original_fc = hw->fc;

	e_dbg("After fix-ups FlowControl is now = %x\n", hw->fc);

	/* Take the 4 bits from EEPROM word 0x0F that determine the initial
	 * polarity value for the SW controlled pins, and setup the
	 * Extended Device Control reg with that info.
	 * This is needed because one of the SW controlled pins is used for
	 * signal detection.  So this should be done before e1000_setup_pcs_link()
	 * or e1000_phy_setup() is called.
	 */
	if (hw->mac_type == e1000_82543) {
		ret_val = e1000_read_eeprom(hw, EEPROM_INIT_CONTROL2_REG,
					    1, &eeprom_data);
		if (ret_val) {
			e_dbg("EEPROM Read Error\n");
			return -E1000_ERR_EEPROM;
		}
		ctrl_ext = ((eeprom_data & EEPROM_WORD0F_SWPDIO_EXT) <<
			    SWDPIO__EXT_SHIFT);
		ew32(CTRL_EXT, ctrl_ext);
	}
#endif

#if 0
	/* Call the necessary subroutine to configure the link. */
	ret_val = (hw->media_type == e1000_media_type_copper) ?
	    e1000_setup_copper_link(hw) : e1000_setup_fiber_serdes_link(hw);
#else
	e1000_setup_copper_link(dev);
#endif

#if 0
	/* Initialize the flow control address, type, and PAUSE timer
	 * registers to their default values.  This is done even if flow
	 * control is disabled, because it does not hurt anything to
	 * initialize these registers.
	 */
	e_dbg("Initializing the Flow Control address, type and timer regs\n");

	ew32(FCT, FLOW_CONTROL_TYPE);
	ew32(FCAH, FLOW_CONTROL_ADDRESS_HIGH);
	ew32(FCAL, FLOW_CONTROL_ADDRESS_LOW);

	ew32(FCTTV, hw->fc_pause_time);

	/* Set the flow control receive threshold registers.  Normally,
	 * these registers will be set to a default threshold that may be
	 * adjusted later by the driver's runtime code.  However, if the
	 * ability to transmit pause frames in not enabled, then these
	 * registers will be set to 0.
	 */
	if (!(hw->fc & E1000_FC_TX_PAUSE)) {
		ew32(FCRTL, 0);
		ew32(FCRTH, 0);
	} else {
		/* We need to set up the Receive Threshold high and low water
		 * marks as well as (optionally) enabling the transmission of
		 * XON frames.
		 */
		if (hw->fc_send_xon) {
			ew32(FCRTL, (hw->fc_low_water | E1000_FCRTL_XONE));
			ew32(FCRTH, hw->fc_high_water);
		} else {
			ew32(FCRTL, hw->fc_low_water);
			ew32(FCRTH, hw->fc_high_water);
		}
	}
#endif
}


static void intele1000_reset(struct pci_dev *dev)
{
	u32 ctrl, manc;

	// e1000_reset_hw()
{

	/* Clear interrupt mask to stop board from generating interrupts */

	e1000_writeR(dev, E1000REG_IMC, 0xffffffff);

	/* Disable the Transmit and Receive units.  Then delay to allow
	 * any pending transactions to complete before we hit the MAC with
	 * the global reset.
	 */
	e1000_writeR(dev, E1000REG_RCTL, 0);
	e1000_writeR(dev, E1000REG_TCTL, E1000_TCTL_PSP);
	e1000_readR(dev, E1000REG_STATUS);
	wait_ms(10);


	/* global reset */
	ctrl = e1000_readR(dev, E1000REG_CTRL);
	printk("e1000 before reset, ctrl: 0x%8x\n", ctrl);
	ctrl |= E1000REG_CTRL_RST;
	e1000_writeR_IO(dev, E1000REG_CTRL, ctrl);
	wait_ms(10);
	printk("e1000 after reset, ctrl: 0x%8x\n", e1000_readR(dev, E1000REG_CTRL));


	/* disable arp */
	manc = e1000_readR(dev, E1000REG_MANC);
	manc &= ~(E1000_MANC_ARP_EN);
	e1000_writeR(dev, E1000REG_MANC, manc);

	e1000_writeR(dev, E1000REG_IMC, 0xffffffff);
	e1000_readR(dev, E1000REG_ICR);
}

	e1000_writeR(dev, E1000REG_WUC, 0);

	// e1000_init_hw
{
	/* Set the media type and TBI compatibility */
	// e1000_set_media_type(hw);

	e1000_writeR(dev, E1000REG_RCTL, E1000REG_RCTL_RST);

	e1000_setup_link(dev);

	/* Set the transmit descriptor write-back policy */
	ctrl = e1000_readR(dev, E1000REG_TXDCTL);
	ctrl = (ctrl & ~E1000_TXDCTL_WTHRESH) | E1000_TXDCTL_FULL_TX_DESC_WB;
	e1000_writeR(dev, E1000REG_TXDCTL, ctrl);

	/* if (adapter->hwflags & HWFLAGS_PHY_PWR_BIT) { */
#if 0
	if (hw->mac_type >= e1000_82544 &&
	    hw->autoneg == 1 &&
	    hw->autoneg_advertised == ADVERTISE_1000_FULL)
#endif
	{
		ctrl = e1000_readR(dev, E1000REG_CTRL);
		/* clear phy power management bit if we are in gig only mode,
		 * which if enabled will attempt negotiation to 100Mb, which
		 * can cause a loss of link at power off or driver unload
		 */
		ctrl &= ~E1000_CTRL_SWDPIN3;
		e1000_writeR(dev, E1000REG_CTRL, ctrl);
	}

	/* enable arp */
	manc = e1000_readR(dev, E1000REG_MANC);
	manc |= E1000_MANC_ARP_EN;
	e1000_writeR(dev, E1000REG_MANC, manc);

}
}


/*
 * | 4K align  |           | padding ...    | 512B align
 * pci_dev     netdev_t                     receive descriptor buff
 */
static int intele1000_devinit(struct pci_dev *dev)
{
    u32 count, ctrl;
    netdev_t *e1000 = (netdev_t *)(dev + 1);
	u32 rdbuff = ((u32)e1000 + 511) & (~0x1FF);
	u32 tdbuff = rdbuff + 512;

    /* get the bar of I/O and bar of MEM */
    for (count = 0; count < ARRAY_ELEMENT_SIZE(dev->pcicfg.bar_mask); count++)
    {
        if ((dev->pcicfg.bar_mask[count]) == 0)
        {
            continue;
        }
        /* I/O BAR */
        if ((dev->pcicfg.u.cfg.baseaddr[count]) & 0x1)
        {
            dev->bar_io = (u16)(dev->pcicfg.u.cfg.baseaddr[count] &
								dev->pcicfg.bar_mask[count]);
        }
        /* MEM BAR */
        else
        {
#if 1
			u32 range = (~(dev->pcicfg.bar_mask[count])) + 1 + PAGE_SIZE;;
			dev->bar_mem[dev->n_bar_mem] =
            	dev->pcicfg.u.cfg.baseaddr[count] & dev->pcicfg.bar_mask[count];
			printk("dev->bar_mem[%d] : 0x%#8x, range: 0x%#8x\n",
				dev->n_bar_mem, dev->bar_mem[dev->n_bar_mem], range);

			if (dev->n_bar_mem == 0)
				mmap(dev->bar_mem[dev->n_bar_mem], dev->bar_mem[dev->n_bar_mem], range, 0);

			(dev->n_bar_mem)++;

#else
        	u32 offset, range = (~(dev->pcicfg.bar_mask[count])) + 1 + PAGE_SIZE;
			u32 va, pa;
            dev->bar_mem = dev->pcicfg.u.cfg.baseaddr[count] &
						   dev->pcicfg.bar_mask[count];
			va = 0x10000000 + 0x1000;
			pa = dev->bar_mem & (~(PAGE_SIZE - 1));

			offset = dev->bar_mem - pa;
			/* let's remap this addr */
			mmap(va, pa, range, 0);

			dev->bar_mem = va + offset;
#endif
        }
    }

	e1000->n_rxdesc = 32;		/* totally 16 receive descriptor */
	e1000->n_txdesc = 32;
	e1000->rd_bufsize = 2048;
	e1000->td_bufsize = 2048;
	e1000->rxdexc_ring = (void *)rdbuff;
	e1000->txdexc_ring = (void *)tdbuff;
	e1000->rxDMA = page_alloc(get_pagerank(e1000->rd_bufsize * e1000->n_rxdesc), MMAREA_LOW1M);
	e1000->txDMA = page_alloc(get_pagerank(e1000->td_bufsize * e1000->n_txdesc), MMAREA_LOW1M);
	intele1000_initrxdsc(e1000, rdbuff, e1000->n_rxdesc);
	intele1000_inittxdsc(e1000, tdbuff, e1000->n_txdesc);
	e1000->tx = intele1000_tx;
	e1000->dev = dev;
	dev->param = e1000;

	intele1000_reset(dev);

	for (count = 0; count < 128; count++)
		e1000_writeR(dev, E1000REG_MTA(count), 0);

	/* MAC addr register */
	e1000->macaddr[0] = (u8)(e1000_macaddr[0] >> 8);
	e1000->macaddr[1] = (u8)(e1000_macaddr[0] >> 0);
	e1000->macaddr[2] = (u8)(e1000_macaddr[1] >> 24);
	e1000->macaddr[3] = (u8)(e1000_macaddr[1] >> 16);
	e1000->macaddr[4] = (u8)(e1000_macaddr[1] >> 8);
	e1000->macaddr[5] = (u8)(e1000_macaddr[1] >> 0);
	e1000_writeR(dev, E1000REG_RAH(0), e1000_macaddr[0] | 0x80000000);
	e1000_writeR(dev, E1000REG_RAL(0), e1000_macaddr[1]);
	// e1000_writeR(dev, E1000REG_RAH(0), 0x5634 | 0x80000000);
	// e1000_writeR(dev, E1000REG_RAL(0), 0x12005452);

	/* receive descriptor config */
{
	ctrl = e1000_readR(dev, E1000REG_RCTL);
	ctrl &= ~E1000REG_RCTL_EN;
	e1000_writeR(dev, E1000REG_RCTL, ctrl);

	e1000_writeR(dev, E1000REG_RDBAL, rdbuff);		/* the rd buff should align to 16 byte */
	e1000_writeR(dev, E1000REG_RDBAH, 0);
	e1000_writeR(dev, E1000REG_RDLEN, e1000->n_rxdesc * 16);/* the rd buff length should align to 128 byte */
	e1000_writeR(dev, E1000REG_RDH, 0);				/* RD header */
	e1000_writeR(dev, E1000REG_RDT, 0);				/* RD tailer */

	/* receive control register, start to receive */
	ctrl &= ~E1000REG_RCTL_RST;
	ctrl |= E1000REG_RCTL_EN;
	ctrl |= E1000REG_RCTL_UPE;
	ctrl |= E1000REG_RCTL_BAM;
	ctrl |= E1000REG_RCTL_SECRC;
	e1000_writeR(dev, E1000REG_RCTL, ctrl);
}
	/* transmit descriptor config */
{
	e1000_writeR(dev, E1000REG_TDBAL, tdbuff);		/* the td buff should align to 16 byte */
	e1000_writeR(dev, E1000REG_TDBAH, 0);
	e1000_writeR(dev, E1000REG_TDLEN, 256);			/* the td buff length should align to 128 byte */
	e1000_writeR(dev, E1000REG_TDH, 0);				/* TD header */
	e1000_writeR(dev, E1000REG_TDT, 0);				/* TD tailer */

	/* transmit control register, start to transmit */
	e1000_writeR(dev, E1000REG_TCTL, 0x4010a);
}

	/* init the int mask */
	e1000_writeR(dev, E1000REG_IMS, 0x1F6DF);

	/* clean all int */
	e1000_readR(dev, E1000REG_ICR);

    /* register the int */
    if (interrup_register(dev->pcicfg.u.cfg.intline, intele1000_isr, dev) != 0)
    {
        printk("intele1000_isr register failed.\n");
        return -1;
    }

	/* for debug */
	dbg_e1000 = dev;

	printk("e1000 init, ctrl: 0x%8x\n", e1000_readR(dev, E1000REG_CTRL));

	return 0;
}


static vendor_device_t spt_vendev[] = {
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82540EM},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82545EM},
};

static pci_drv_t intele1000_drv = {
    .drvname = "intele1000_drv",
    .vendev = spt_vendev,
    .n_vendev = ARRAY_ELEMENT_SIZE(spt_vendev),
    .pci_init = intele1000_devinit,
};


/* this is the driver for realtek ethernet chip 10EC:8139 */
static void __init intele1000init()
{
    /* register the pci drv */
    pcidrv_register(&intele1000_drv);
}

module_init(intele1000init, 5);

static void cmd_e1000op_opfunc(char *argv[], int argc, void *param)
{
    u32 count;
    if (argc == 1)
    {
        for (count = 0; count < ARRAY_ELEMENT_SIZE(e1000_intstat); count++)
        {
            if ((count % 4) == 0)
                printk("\n");

            printk("%2d-->%4d,    ", count, e1000_intstat[count]);
        }
        printk("\n");
    }
    else if (argc == 3)
    {
		if (memcmp(argv[1], "-r", sizeof("-r")) == 0)
		{
			u32 v, off = str2num(argv[2]);
			v = e1000_readR(dbg_e1000, off);
			printk("off 0x%#x: 0x%#x.\n", off, v);
		}
		else if (memcmp(argv[1], "-rp", sizeof("-rp")) == 0)
		{
			u32 off = str2num(argv[2]);
			u16 v;
			e1000_read_phy_reg(dbg_e1000, off, &v);
			printk("phy off 0x%#x: 0x%#x.\n", off, v);
		}
		else
		{
			printk("invalid command.\n");
			return;
		}

	}
	else if (argc == 4)
	{
		if (memcmp(argv[1], "-w", sizeof("-w")) == 0)
		{
			u32 v, off;
			off = str2num(argv[2]);
			v = str2num(argv[3]);
			e1000_writeR(dbg_e1000, off, v);
			printk("0x%#x --> off 0x%#x.\n", v, off);
		}
		else if (memcmp(argv[1], "-diag", sizeof("-diag")) == 0)
		{
			u32 i, base, n, v;
			base = str2num(argv[2]);
			n = str2num(argv[3]);
			for (i = 0; i < n; i++)
			{
				v = e1000_readR(dbg_e1000, base + i * 4);
				if (v)
					printk("off 0x%#x: 0x%#x.\n", base + i * 4, v);
			}
			printk("read 0x%#6x ~ 0x%#6x\n", base, base + n * 4 - 1);
		}
		else
		{
			printk("invalid command.\n");
			return;
		}
	}
	else
    {
        printk("invalid command.\n");
    }
}

struct command cmd_e1000op _SECTION_(.array.cmd) =
{
    .cmd_name   = "e1000op",
    .info       = "e1000 operation.e1000op, -r off, -w off value",
    .op_func    = cmd_e1000op_opfunc,
};

