elementclass MBarkGateway { 
    $rule | input 
    -> ProtocolTranslator46
    -> MBArkFirewall(FILENAME $rule)
    -> proxy::MBArkProxy;
    proxy[0] -> [0]output;
    proxy[1] -> [1]output;
}

c :: Classifier(
    12/0800, 
    -);

fd::FromDump(FILENAME /tmp/test.pcap, STOP true);

//fd::FromDevice(en0) 

fd  -> c

c[0] 
    -> Strip(14)
    -> gw0::MBarkGateway(conf/fw.rules)

c[1]
    -> Discard

gw0[0] 
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> Discard

gw0[1]
    -> Discard