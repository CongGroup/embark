rw :: IPRewriter(pattern 8.0.0.2 9555 - - 0 1);

pd1::PollDevice(p3p1, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> [0]rw;
rw[0] -> q1::SimpleQueue() -> sd1::SendDevice(eth7, BURST 32, QUEUE 0);
rw[1] -> dd::Discard();

//pd2::PollDevice(p3p2, QUEUE 0, BURST 32) -> Strip(14) -> CheckIPHeader2() -> MarkIPHeader() -> q2::SimpleQueue() -> sd2:\
:SendDevice(eth6, BURST 32, QUEUE 0);


StaticThreadSched(pd1 0, q1 0, sd1 0);
//StaticThreadSched(pd2 1, q2 1, sd2 1);

