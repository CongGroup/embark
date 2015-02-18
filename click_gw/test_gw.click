FromDevice(DEVNAME en0)
    -> SetTimestamp
    -> Strip(14)
    -> chk_ip :: CheckIPHeader;

chk_ip[0]
//    -> IPPrint
    -> ProtocolTranslator46
    -> chk_ip6 :: CheckIP6Header
//    -> IP6Print
    -> MBArkGateway
//    -> IP6Print
    -> accum :: TimestampAccum
    -> Discard;

chk_ip[1]
    -> Discard;

chk_ip6[1]
    -> Discard;