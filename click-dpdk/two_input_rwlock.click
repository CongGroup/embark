rw :: NBSynchIPRewriter(pattern 1.2.3.4-1.2.3.6 49152-65535# - - 0 1);
//rw :: Null;

rw[2] -> Idle;
rw[1] -> Discard;

Idle -> [1]rw;

//mtmq::ThreadSafeQueue(20000);
mtmq:: MultiThreadMultiQueue(20000);

//Input Devices
pd0::PollDevice(p3p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> EtherEncap(0x800, 66:55:44:33:22:11, 66:55:44:33:22:11) -> [0]rw;

pd1::PollDevice(p4p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> EtherEncap(0x800, 66:55:44:33:22:11, 66:55:44:33:22:11) -> [0]rw;
//The lock-log packets

rw[0] -> mtmq;
//dp::ToDevice(p3p2);
dp0::SendDevice(p4p1, BURST 32, QUEUE 0)
dp1::SendDevice(p4p1, BURST 32, QUEUE 1)

//The data plane goes out to the devicemtmq[0] -> dp0;
mtmq[0] -> dp0;
mtmq[1] -> dp1; 


//Thread Schedule
StaticThreadSched(pd0 0, dp0 0, pd1 1, dp1 1);
