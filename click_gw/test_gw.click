FromDevice(DEVNAME en0)
    -> Strip(14)
    -> chk_ip :: CheckIPHeader;

chk_ip[0]
    -> IPPrint
    -> ProtocolTranslator46
    -> IP6Print
    -> MBArkGateway
    -> Discard;

chk_ip[1]
    -> Discard;