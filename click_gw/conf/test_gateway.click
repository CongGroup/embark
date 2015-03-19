tbl :: MBarkTable

elementclass MBarkGateway { 
    $rule | input 
    -> MBArkFirewall(FILENAME $rule, ENABLE false, TABLE tbl, V4 true)
    -> proxy::MBArkProxy(V4 true);
    proxy[0] -> [0]output;
    proxy[1] -> [1]output;
}

c :: Classifier(
    12/0800, 
    -);

//fd::FromDump(FILENAME /tmp/test.pcap, STOP true);

fd0::FromDevice(eth1, SNIFFER false)
sd0::ToDevice(eth2) 

fd0  -> c

q::SimpleQueue()

c[0] 
    -> Strip(14)
    -> gw0::MBarkGateway(conf/fw.rules)

c[1]
    -> Strip(14)
    -> EtherEncap(0x0800, 08:00:27:62:3c:7d, 08:00:27:88:bc:14)
    -> q
    -> sd0

gw0[0] 
    -> EtherEncap(0x0800, 08:00:27:62:3c:7d, 08:00:27:88:bc:14)
    -> IPPrint(TCP)
    -> q
    -> sd0

gw0[1]
    -> EtherEncap(0x0800, 08:00:27:62:3c:7d, 08:00:27:88:bc:14)
    -> IPPrint(UDP)
    -> q
    -> sd0


c2 :: Classifier(
    12/0800, 
    -);
fd1::FromDevice(eth2, SNIFFER false) -> c2
sd1::ToDevice(eth1) 

q2::SimpleQueue -> sd1

c2[0] 
    -> Strip(14)
    -> gwrev0::MBArkFirewallRev(TABLE tbl, V4 true)
    -> EtherEncap(0x0800, 08:00:27:2a:38:fc, 08:00:27:88:ec:69)
    -> IPPrint(TCP)
    -> q2

c2[1]
    -> Strip(14)
    -> EtherEncap(0x0800, 08:00:27:2a:38:fc, 08:00:27:88:ec:69)
    -> q2