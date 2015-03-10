

dp:: SimpleQueue(20000) -> SendDevice(p4p2, BURST 32, QUEUE 0)
//Input Devices
pd1::PollDevice(p4p1, QUEUE 0, BURST 32) ->dp;
pd2::PollDevice(p3p1, QUEUE 0, BURST 32) -> dp;
//Wrap IPRewriter in Synchqueue

//Thread Schedule
StaticThreadSched(pd1 0, pd2 1, dp 2);
