pd0::PollDevice(p513p2, QUEUE 0, BURST 32) 
rw :: IPRewriter(pattern 1.1.1.1 1024-65535 - - 0 1);
q::SimpleQueue(20000);
sd0::SendDevice(p513p2, QUEUE 0, BURST 32);

pd0 -> rw;
rw[0] -> q -> sd0;
rw[1] -> q -> sd0;