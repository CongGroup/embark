

dp:: ThreadSafeQueue(20000) -> EtherEncap(0x800, 66:55:44:33:22:11, 66:55:44:33:22:11) -> SendDevice(p3p1, BURST 32, QUEUE 0)


pd1::PollDevice(p3p1, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> dp;
pd2::PollDevice(p4p1, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> dp;

//Thread Schedule
StaticThreadSched(pd1 0, pd2 1, dp 0);
