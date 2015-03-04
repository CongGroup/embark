ether_classifier :: Classifier(12/0800,-); // Filter out L2 traffic 

ether_classifier[1] -> Discard;

myelement::SynchQueueWrapper;

//rewriter :: IPRewriter(
//  pattern 1.2.3.4-1.2.3.6 49152-65535 - - 0 1 
//);

//rewriter[1] -> Discard;

myelement[0] -> Queue(20000) -> UDPIPEncap(1.0.0.1, 1234, 2.0.0.2, 1234) -> ToDump("kittens.pcap");

FromDump("lbl.pcap", TIMING 1)
    -> ether_classifier[0]         // For all incoming IP traffic
    -> Strip(14)            // strip off Ethernet
    -> CheckIPHeader2()      // Checksum
    -> MarkIPHeader()
    //-> IPPrint("IP Packet")
    //-> rewriter[0]
    //-> IPPrint("Rewritten")
    -> myelement[1]
    -> TimedUnqueue(10)
    -> Discard;
