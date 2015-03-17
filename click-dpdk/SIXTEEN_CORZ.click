rw :: SynchIPRewriter(pattern 1.2.3.4-1.2.3.6 49152-65535# - - 0 1);

rw[2] ->SimpleQueue(20000) -> do::Discard;
rw[1] -> Discard;

Idle -> [1]rw;
rwchain::Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> EtherEncap(0x800, 66:55:44:33:22:11, 66:55:44:33:22:11) -> [0]rw;

//mtmq::ThreadSafeQueue(20000);
mtmq:: MultiThreadMultiQueue(20000);

//Input Devices
pd0::PollDevice(p4p1, QUEUE 0, BURST 32) -> rwchain;
pd1::PollDevice(p4p1, QUEUE 1, BURST 32) -> rwchain;
pd2::PollDevice(p4p1, QUEUE 2, BURST 32) -> rwchain;
pd3::PollDevice(p4p1, QUEUE 3, BURST 32) -> rwchain;
pd4::PollDevice(p4p1, QUEUE 4, BURST 32) -> rwchain;
pd5::PollDevice(p4p1, QUEUE 5, BURST 32) -> rwchain;
pd6::PollDevice(p4p1, QUEUE 6, BURST 32) -> rwchain;
pd7::PollDevice(p4p1, QUEUE 7, BURST 32) -> rwchain;
pd8::PollDevice(p4p1, QUEUE 8, BURST 32) -> rwchain;
pd9::PollDevice(p4p1, QUEUE 9, BURST 32) -> rwchain;
//pd10::PollDevice(p4p1, QUEUE 10, BURST 32) -> rwchain;
//pd11::PollDevice(p4p1, QUEUE 11, BURST 32) -> rwchain;
//pd12::PollDevice(p4p1, QUEUE 12, BURST 32) -> rwchain;
//pd13::PollDevice(p4p1, QUEUE 13, BURST 32) -> rwchain;
//pd14::PollDevice(p4p1, QUEUE 14, BURST 32) -> rwchain;
//pd15::PollDevice(p4p1, QUEUE 15, BURST 32) -> rwchain;


//The lock-log packets

rw[0] -> mtmq;
//dp::ToDevice(p3p2);
dp0::SendDevice(p4p1, BURST 32, QUEUE 0)
dp1::SendDevice(p4p1, BURST 32, QUEUE 1)
dp2::SendDevice(p4p1, BURST 32, QUEUE 2)
dp3::SendDevice(p4p1, BURST 32, QUEUE 3)
dp4::SendDevice(p4p1, BURST 32, QUEUE 4)
dp5::SendDevice(p4p1, BURST 32, QUEUE 5)
dp6::SendDevice(p4p1, BURST 32, QUEUE 6)
dp7::SendDevice(p4p1, BURST 32, QUEUE 7)
dp8::SendDevice(p4p1, BURST 32, QUEUE 8)
dp9::SendDevice(p4p1, BURST 32, QUEUE 9)
//dp10::SendDevice(p4p1, BURST 32, QUEUE 10)
//dp11::SendDevice(p4p1, BURST 32, QUEUE 11)
//dp12::SendDevice(p4p1, BURST 32, QUEUE 12)
//dp13::SendDevice(p4p1, BURST 32, QUEUE 13)
//dp14::SendDevice(p4p1, BURST 32, QUEUE 14)
//dp15::SendDevice(p4p1, BURST 32, QUEUE 15)


//The data plane goes out to the devicemtmq[0] -> dp0;
mtmq[0] -> dp0;
mtmq[1] -> dp1;
mtmq[2] -> dp2;
mtmq[3] -> dp3;
mtmq[4] -> dp4;
mtmq[5] -> dp5;
mtmq[6] -> dp6;
mtmq[7] -> dp7;
mtmq[8] -> dp8;
mtmq[9] -> dp9;
//mtmq[10] -> dp10;
//mtmq[11] -> dp11;
//mtmq[12] -> dp12;
//mtmq[13] -> dp13;
//mtmq[14] -> dp14;
//mtmq[15] -> dp15;

//Thread Schedule
StaticThreadSched(pd0 0, dp0 0, pd1 1, dp1 1, pd2 2, dp2 2, pd3 3, dp3 3, pd4 4, dp4 4, pd5 5, dp5 5, pd6 6, dp6 6, pd7 7, dp7 7, pd8 8, dp8 8, pd9 9, dp9 9, do 10);
