FromDevice(DEVNAME en0)
    -> Strip(14)
    -> chk_ip :: CheckIPHeader;

chk_ip[0]
    -> IPPrint
    -> ProtocolTranslator46
    -> chk_ip6 :: CheckIP6Header
    -> IP6Print
    -> MBArkGateway
    -> IP6Print
    -> Discard;

chk_ip[1]
    -> Discard;

chk_ip6[1]
    -> Discard;