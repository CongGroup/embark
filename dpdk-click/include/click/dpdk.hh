#ifndef CLICK_DPDK_HH
#define CLICK_DPDK_HH

//dpdk const
#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1
#define L2FWD_MAX_PORTS 32
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
#define NB_MBUF   8192
#define NUM_RX_QUEUE 2
#define NUM_TX_QUEUE 2

/*                                                                                                  
* RX and TX Prefetch, Host, and Write-back threshold values should be
 * carefully set for optimal performance. Consult the network  
 * controller's datasheet and supporting DPDK documentation for guidance
 * on how these parameters should be set.     
 */
#define RX_PTHRESH 16 //8 /**< Default values of RX prefetch threshold reg. */
#define RX_HTHRESH 8 /**< Default values of RX host threshold reg. */
#define RX_WTHRESH 8  /**< Default values of RX write-back threshold reg. */

/*                                                                                         
 * These default values are optimized for use with the Intel(R) 82599 10 GbE
 * Controller and the DPDK ixgbe PMD. Consider using other values for other     
 * network controllers and/or network drivers.          
 */
#define TX_PTHRESH 48 //36 /**< Default values of TX prefetch threshold reg. */
#define TX_HTHRESH 16 //0  /**< Default values of TX host threshold reg. */
#define TX_WTHRESH 0  /**< Default values of TX write-back threshold reg. */

#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN 200000ULL /* around 100us at 2 Ghz */

#define SOCKET0 0

/*                                                                      
 * Configurable number of RX/TX ring descriptors                          
 */
#define RTE_TEST_RX_DESC_DEFAULT 512 //128
#define RTE_TEST_TX_DESC_DEFAULT 512
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;


static struct rte_eth_conf port_conf;
static const struct rte_eth_rxconf rx_conf = { {RX_PTHRESH, RX_HTHRESH, RX_WTHRESH}, 0 };
static const struct rte_eth_txconf tx_conf = { {TX_PTHRESH, TX_HTHRESH, TX_WTHRESH}, 0, 0 };

//struct rte_mempool * pktmbuf_pool = NULL;


#endif
