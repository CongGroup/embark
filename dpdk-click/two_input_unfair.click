rw :: IPRewriter(pattern 1.2.3.4-1.2.3.6 49152-65535 - - 0 1);
rw2 :: IPRewriter(pattern 1.2.3.4-1.2.3.6 49152-65535 - - 0 1);
rw[1] -> dd::Discard;
rw2[1] -> Discard;

tsq :: ThreadSafeQueue(20000000);

//Input Devices
pd1::PollDevice(p3p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> rw -> tsq;
pd2::PollDevice(p4p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> rw2 -> tsq;

dp::SendDevice(p3p1, BURST 32, QUEUE 0)

//The data plane goes out to the device
tsq ->  EtherEncap(0x800, 66:55:44:33:22:11, 66:55:44:33:22:11) -> dp; 

//Thread Schedule
StaticThreadSched(pd1 0, pd2 1, dp 2);
