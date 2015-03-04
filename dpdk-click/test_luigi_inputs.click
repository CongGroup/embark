pipeline::LockingPushNull -> LockingPushNull -> PushNull -> PushNull -> tf::ThreadFanner;
i0::InfiniteSource(LENGTH 64, BURST 64) -> pipeline;
tf[0] -> d0::Discard;
i1::InfiniteSource(LENGTH 64, BURST 64) -> pipeline;
tf[1] -> d1::Discard;

DriverManager(
    set a0 0,
    set a1 0,
    label x, 
    wait 1s,

    set b0 $(d0.count),
    print "core0 done $(sub $b0 $a0) packets in 1s",
    set a0 $b0,

    set b1 $(d1.count),
    print "core1 done $(sub $b1 $a1) packets in 1s",
    set a1 $b1,

    goto x 1
);
StaticThreadSched(i0 0, d0 0, i1 1, d1 1)
