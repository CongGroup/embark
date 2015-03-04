//PollDevice(eth4, QUEUE 0, BURST 32) -> SimpleQueue(200000) -> DelayShaper(0.010) -> SendDevice(eth4, QUEUE 0, BURST 32);


//pd1::PollDevice(eth4, QUEUE 0, BURST 16) -> SimpleQueue(150) -> sd1::SendDevice(eth4, QUEUE 0, BURST 16);
pd2::PollDevice(eth5, QUEUE 0, BURST 16) -> SimpleQueue(150) -> sd2::SendDevice(eth5,QUEUE 0, BURST 16);
pd3::PollDevice(eth12, QUEUE 0, BURST 16)-> SimpleQueue(150) -> sd3::SendDevice(eth12, QUEUE 0, BURST 16);

StaticThreadSched(pd2 1, sd2 1, pd3 2, sd2 2);
