rw :: IPRewriter(pattern 1.2.3.4-1.2.3.6 49152-65535 - - 0 1);
rw[1] -> dd::Discard;

tsq::SimpleQueue(150);

//Input Devices
pd1::PollDevice(eth4, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> rw -> tsq;

dp::SendDevice(eth4, BURST 32, QUEUE 0)

//The data plane goes out to the device
tsq ->  EtherEncap(0x800, 66:55:44:33:22:11, 66:55:44:33:22:11) -> dp; 

//Thread Schedule
StaticThreadSched(pd1 0, dp 0);
