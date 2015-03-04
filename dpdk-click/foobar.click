// mazu-nat.click

// ADDRESS INFORMATION

// DEVICE SETUP

elementclass GatewayDevice {
  $device |
  from :: PollDevice($device)
	-> output;
  input -> q :: SimpleQueue(1024)
	-> to :: SendDevice($device);
}

// The following version of GatewayDevice sends a copy of every packet to
// DiscardSniffers, so that you can use tcpdump(1) to debug the gateway.

elementclass SniffGatewayDevice {
  $device, $queue |
  from :: PollDevice($device, QUEUE $queue, BURST 32)
	-> output;
  input -> q :: SimpleQueue(1024)
	-> to :: SendDevice($device, QUEUE $queue, BURST 32);
}

extern_dev :: SniffGatewayDevice(eth4, 0);
//extern_dev2 :: SniffGatewayDevice(eth4, 1);
intern_dev :: SniffGatewayDevice(eth6, 0);
//intern_dev2 :: SniffGatewayDevice(eth6, 1);



extern_dev -> intern_dev;
//extern_dev2 -> intern_dev2;
intern_dev -> extern_dev;
//intern_dev2 -> extern_dev2;

StaticThreadSched(extern_dev/from 0, intern_dev/to 0, extern_dev/to 1, intern_dev/from 1)

//StaticThreadSched(extern_dev/from 0, extern_dev/to 1, extern_dev2/from 2, extern_dev2/to 3, intern_dev/from 1, intern_dev/to 0, intern_dev2/from 3, intern_dev2/to 2)

