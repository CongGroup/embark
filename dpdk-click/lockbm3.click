//PollDevice(eth12, QUEUE 0) -> Print -> Queue(200) -> SendDevice(eth12, QUEUE 0)

stuff:: LockingPushNull-> LockingPushNull -> LockingPushNull  -> LockingPushNull  -> PushNull-> PushNull -> PushNull -> PushNull -> tf::ThreadFanner;

//m:: MultiThreadMultiQueue(200)


//p0:: PollDevice(eth12, QUEUE 0, BURST 32) -> stuff -> m[0]  -> s0:: SendDevice(eth12, QUEUE 0, BURST 32);
//p1:: PollDevice(eth12, QUEUE 0, BURST 32) -> stuff -> m[1]  -> s1:: SendDevice(eth12, QUEUE 0, BURST 32);


p0:: PollDevice(eth4, QUEUE 0, BURST 32) -> stuff;
tf[0]  -> SimpleQueue(200)-> s0:: SendDevice(eth4, QUEUE 0, BURST 32);
p1:: PollDevice(eth5, QUEUE 0, BURST 32) -> stuff;
tf[1]  -> SimpleQueue(200) -> s1:: SendDevice(eth5, QUEUE 0, BURST 32);
p2:: PollDevice(eth6, QUEUE 0, BURST 32) -> stuff;
tf[2] -> SimpleQueue(200)  -> s2:: SendDevice(eth6, QUEUE 0, BURST 32);


StaticThreadSched(p0 0, s0 0, p1 1, s1 1, p2 2, s2 2);
