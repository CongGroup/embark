// testdevice.click

// Tests whether Click can read packets from the network.
// You may need to modify the device name in FromDevice.
// You'll probably need to be root to run this.

// Run with
//    click testdevice.click
// (runs as a user-level program; uses Linux packet sockets or a similar
// mechanism), or
//    click-install testdevice.click
// (runs inside a Linux kernel).

// If you run this inside the kernel, your kernel's ordinary IP stack
// will stop getting packets from eth0. This might not be convenient.
// The Print messages are printed to the system log, which is accessible
// with 'dmesg' and /var/log/messages. The most recent 2-4K of messages are
// stored in /click/messages.

p0 :: FromDevice(eth4) 
p2 :: Print(ok); 
p1 :: Discard;
p0 -> p2 -> p1;
StaticThreadSched(p0 0);
StaticThreadSched(p2 1);
//pd1::FromDevice(eth5) -> Print(ok) -> Discard;

StaticThreadSched(p1 2);
