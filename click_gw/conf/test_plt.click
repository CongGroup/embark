tbl :: MBarkTable
tun1 :: KernelTun(10.9.0.1/24, GATEWAY 10.9.0.2, DEVNAME tun1)
//fw :: MBArkFirewall(FILENAME conf/fw.rules, ENABLE false, TABLE tbl, V4 true)


q :: SimpleQueue()
ether :: EtherEncap(0x0800, 08:00:27:9e:dd:15, 52:54:00:12:35:02)

tun1 -> Paint(1, 19) -> FixIPSrc(10.8.0.4) -> SetUDPChecksum -> q;
q -> IPPrint(ToTun) -> ToDevice(tun0);

FromDevice(tun0, SNIFFER false) 
    -> Paint(1, 19) 
    -> FixIPSrc(10.0.2.15)
    -> SimpleQueue
    -> ether
    -> IPPrint(ToInternet)
    -> ToDevice(eth0)