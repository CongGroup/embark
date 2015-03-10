rw :: SynchIPRewriter(pattern 1.2.3.4-1.2.3.6 49152-65535# - - 0 1);
//rw :: Null;

//Input Devices
pd1::PollDevice(p3p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> [0]rw;
pd2::PollDevice(p4p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> [0]rw;

//The lock-log packets
//dpo::SendDevice(p3p1, BURST 32, QUEUE 0)
dpo::ToDump("controlpackets.pcap");
rw[2] -> SimpleQueue(200000) -> UDPIPEncap(1.0.0.1, 1234, 2.0.0.2, 1234) -> EtherEncap(0x800, 11:22:33:44:55:66, 11:22:33:44:55:66) -> dpo;

//Wrap IPRewriter in Synchqueue
rw[1] -> dd::Discard;

// Entering the log packets
Idle -> [1]rw;
//rp::FromDump("replay_dump.pcap") -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> StripIPHeader() -> [1]rw;

// Dataplane packets
//dp::ToDevice(p3p2);
dp::SendDevice(p3p2, BURST 32, QUEUE 0)
//dp::ToDump("datapackets.pcap"); 
//The data plane goes out to the device
rw -> EtherEncap(0x800, 66:55:44:33:22:11, 66:55:44:33:22:11) -> ThreadSafeQueue(200000) -> dp; 

//Thread Schedule
//StaticThreadSched(pd1 0, pd2 1, dp 2, dpo 3);
