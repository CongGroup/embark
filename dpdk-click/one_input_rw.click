
rw :: IPRewriter(pattern 1.2.3.4-1.2.3.6 49152-65535 - - 0 1);

rw[1] -> Discard;
dp:: MultiThreadMultiQueue(20000) -> EtherEncap(0x800, 11:22:33:44:55:66, 11:22:33:44:55:66) -> SendDevice(p3p2, BURST 32, QUEUE 0)
//Input Devices
pd1::PollDevice(p4p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> rw[0] ->dp;

//pd2::PollDevice(p3p2, QUEUE 0, BURST 32) -> dp;
//Wrap IPRewriter in Synchqueue
//rw[1] -> dd::Discard

//Thread Schedule
StaticThreadSched(pd1 0, dp 0);
