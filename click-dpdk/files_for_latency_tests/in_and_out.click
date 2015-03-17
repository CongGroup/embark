//PollDevice(eth4, QUEUE 0, BURST 32) -> SimpleQueue(200000) -> DelayShaper(0.010) -> SendDevice(eth4, QUEUE 0, BURST 32);

PollDevice(eth4, QUEUE 0, BURST 32) -> SimpleQueue(300)  -> SendDevice(eth4, QUEUE 0, BURST 32);
