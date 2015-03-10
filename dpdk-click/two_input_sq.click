rw :: IPRewriter(pattern 1.2.3.4-1.2.3.6 49152-65535 - - 0 1);
//rw :: Null;
synchqueue::SynchQueueWrapper(INLOG "controlpackets.pcap", CAPACITY 200000);

//Input Devices
pd1::PollDevice(p3p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> [0]synchqueue;
pd2::PollDevice(p4p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> [0]synchqueue;


//The lock-log packets
locklog::Queue(20000) -> UDPIPEncap(1.0.0.1, 1234, 2.0.0.2, 1234) -> EtherEncap(0x800, 11:22:33:44:55:66, 11:22:33:44:55:66) -> ToDump("controlpackets.pcap");
synchqueue[0] -> locklog; 


//Wrap IPRewriter in Synchqueue
synchqueue[1] -> rw[0] -> [1]synchqueue;
rw[1] -> dd::Discard;

dp::SendDevice(p3p2, BURST 32, QUEUE 0)
//dp::ToDump("datapackets.pcap"); 
//The data plane goes out to the device
synchqueue[2] -> EtherEncap(0x800, 66:55:44:33:22:11, 66:55:44:33:22:11) -> dp; 

//Thread Schedule
StaticThreadSched(pd1 0, pd2 1, dp 2, locklog 3);
