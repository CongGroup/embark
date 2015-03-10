m8::ThreadFanner;
m0::PushNull -> PushNull -> PushNull -> PushNull -> PushNull -> PushNull -> PushNull -> PushNull ->m8;
i0::InfiniteSource(LENGTH 64, BURST 64) -> m0;
m8[0] -> d0::Discard;
i1::InfiniteSource(LENGTH 64, BURST 64) -> m0;
m8[1] -> d1::Discard;
i2::InfiniteSource(LENGTH 64, BURST 64) -> m0;
m8[2] -> d2::Discard;

DriverManager(
    set a0 0,
    set a1 0,
    set a2 0,
    label x, 
    wait 1s,

    set b0 $(d0.count),
    print "core0 done $(sub $b0 $a0) packets in 1s",
    set a0 $b0,

    set b1 $(d1.count),
    print "core1 done $(sub $b1 $a1) packets in 1s",
    set a1 $b1,

    set b2 $(d2.count),
    print "core2 done $(sub $b2 $a2) packets in 1s",
    set a2 $b2,

    goto x 1
);
StaticThreadSched(i0 0, d0 0, i1 1, d1 1, i2 2, d2 2)
