tbl :: MBarkTable

c :: Classifier(
    12/0800, 
    -);

elementclass MBarkGateway {
    $rule | input 
//    -> ProtocolTranslator46
    -> MBArkFirewall(FILENAME $rule, TABLE tbl, V4 true, ENABLE false, STATEFUL false)
    -> proxy::MBArkProxy(V4 true);
    dpi::MBArkDPI;
    proxy[0] -> dpi;
    dpi[0] -> [0]output;
    dpi[1] -> [1]output;
    proxy[1] -> [1]output;
}

//FromDump(FILENAME /project/cs/netsys/data/pcaps/m57/m57.pcap, STOP true) -> c;

pd0::PollDevice(p513p1, QUEUE 0, BURST 32) -> c

q0::SimpleQueue(20000)
    -> sd0::SendDevice(p513p2, QUEUE 0, BURST 32);

c[0] 
    -> Strip(14)
    -> gw0::MBarkGateway(conf/test_fw.rules)

gw0[0]    
    -> AESForward
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> q0

gw0[1]
    -> Discard

c[1]
    -> AESForward
    -> q0