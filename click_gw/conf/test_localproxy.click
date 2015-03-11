elementclass MBarkGateway { 
    $rule | input 
    -> ProtocolTranslator46
    -> MBArkFirewall(FILENAME $rule)
    -> Print
    -> MBArkProxy
    -> output
}

fd::FromDump(FILENAME /tmp/trace.pcap, STOP true);

//fd0::FromDevice(en0) 

fd  -> Strip(14)
    -> gw0::MBarkGateway(conf/fw.rules)
    -> EtherEncap(0x86DD, 1:1:1:1:1:1, 2:2:2:2:2:2)
    -> Discard