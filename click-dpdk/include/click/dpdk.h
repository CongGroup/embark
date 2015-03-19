/* Created 2012-2013 by Maziar Manesh, Intel Labs*/
#ifndef CLICK_DPDK_HH
#define CLICK_DPDK_HH

#ifdef __cplusplus
extern "C" {
#endif

//dpdk const
#define RTE_LOGTYPE_DPDK RTE_LOGTYPE_USER1
#define DPDK_MAX_PORTS 32
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
#define NB_MBUF   8192 * 2

/* Multi-queue and RSS related configs, set CLICK_RSS_ON to 1 if you need to turn on the 
   feature and set RX and TX number of queues > 1, ow, set RX queue numbers to 1
   tx queue numbes can be set more than 1 without rss for lockless ops
*/
#define CLICK_RSS_ON 0 

#define NUM_RX_QUEUE 1
#define NUM_TX_QUEUE 1

//following values configure rx unit operations in hw in the NIC for some offloading to the hw
//values should not be changed here unless you have read the NIC's programming
//manual, DPDK user guide for setting these and know what you are doing
/**< CRC stripped by hardware in the NIC*/
#define HW_STRIP_CRC 1
/**< turn on IP checksum calc by hardware in the NIC*/
#define HW_IP_CHECKSUM 0
#define SPLIT_HDR_SIZE 0
#define HEADER_SPLIT 0
#define HW_VLAN_FILTER 0
#define JUMBO_FRAME 0


/*                                            
* RX and TX Prefetch, Host, and Write-back threshold values should be
 * carefully set for optimal performance. Consult the network  
 * controller's datasheet and supporting DPDK documentation for guidance
 * on how these parameters should be set.     
 */
#define RX_PTHRESH 8 //16 /**< Default values of RX prefetch threshold reg. */
#define RX_HTHRESH 8 /**< Default values of RX host threshold reg. */
#define RX_WTHRESH 4 //8  /**< Default values of RX write-back threshold reg. */

/*                                                                                         
 * These default values are optimized for use with the Intel(R) 82599 10 GbE
 * Controller and the DPDK ixgbe PMD. Consider using other values for other     
 * network controllers and/or network drivers.          
 */
#define TX_PTHRESH 36 //48 //36 /**< Default values of TX prefetch threshold reg. */
#define TX_HTHRESH 0  //16 //0  /**< Default values of TX host threshold reg. */
#define TX_WTHRESH 0  /**< Default values of TX write-back threshold reg. */

#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN 200000ULL /* around 100us at 2 Ghz */

#define SOCKET0 0

/*                                                                      
 * CONFIGURABLE number of RX/TX ring descriptors                          
 */
#define RTE_TEST_RX_DESC_DEFAULT 512 //128
#define RTE_TEST_TX_DESC_DEFAULT 512
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;


  //static struct rte_eth_conf port_conf;
static const struct rte_eth_rxconf rx_conf = { {RX_PTHRESH, RX_HTHRESH, RX_WTHRESH}, 0 };
static const struct rte_eth_txconf tx_conf = { {TX_PTHRESH, TX_HTHRESH, TX_WTHRESH}, 0, 0 };

//struct rte_mempool * pktmbuf_pool = NULL;

#ifdef __cplusplus
}
#endif

#endif
