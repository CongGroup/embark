//PollDevice(eth12, QUEUE 0) -> Print -> Queue(200) -> SendDevice(eth12, QUEUE 0)

stuff:: Null-> Null -> Null -> Null -> Null -> Null -> Null -> Null -> tf::ThreadFanner;

//m:: MultiThreadMultiQueue(200)


//p0:: PollDevice(eth12, QUEUE 0, BURST 32) -> stuff -> m[0]  -> s0:: SendDevice(eth12, QUEUE 0, BURST 32);
//p1:: PollDevice(eth12, QUEUE 0, BURST 32) -> stuff -> m[1]  -> s1:: SendDevice(eth12, QUEUE 0, BURST 32);


p0:: PollDevice(eth4, QUEUE 0, BURST 32) -> stuff;
tf[0]  -> SimpleQueue(200)-> s0:: SendDevice(eth4, QUEUE 0, BURST 32);


StaticThreadSched(p0 0, s0 0);
