elementclass MBarkGateway {
    $rule | input 
    -> ProtocolTranslator46
    -> MBArkFirewall(FILENAME $rule)
    -> proxy::MBArkProxy;
    proxy[0] -> [0]output;
    proxy[1] -> [1]output;
}

//FromDump(FILENAME /project/cs/netsys/data/pcaps/m57/m57.pcap, STOP true) -> c;

pd0::PollDevice(p513p1, QUEUE 0, BURST 32) -> Strip(14)
    -> gw0::MBarkGateway(conf/test_fw.rules)

q0::SimpleQueue(20000)
    -> sd0::SendDevice(p513p2, QUEUE 0, BURST 32);

gw0[0]    
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q0

gw0[1]
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q0

pd1::PollDevice(p513p1, QUEUE 1, BURST 32) -> Strip(14)
    -> gw1::MBarkGateway(conf/test_fw.rules)

q1::SimpleQueue(20000)
    -> sd1::SendDevice(p513p2, QUEUE 1, BURST 32);

gw1[0]    
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q1

gw1[1]
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q1

pd2::PollDevice(p513p1, QUEUE 2, BURST 32) -> Strip(14)
    -> gw2::MBarkGateway(conf/test_fw.rules)

q2::SimpleQueue(20000)
    -> sd2::SendDevice(p513p2, QUEUE 2, BURST 32);

gw2[0]    
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q2

gw2[1]
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q2

pd3::PollDevice(p513p1, QUEUE 3, BURST 32) -> Strip(14)
    -> gw3::MBarkGateway(conf/test_fw.rules)

q3::SimpleQueue(20000)
    -> sd3::SendDevice(p513p2, QUEUE 3, BURST 32);

gw3[0]    
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q3

gw3[1]
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q3


StaticThreadSched(pd0 1, pd1 2, pd2 3, pd3 4);
StaticThreadSched(sd0 5, sd1 6, sd2 7, sd3 8);