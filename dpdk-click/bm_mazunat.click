// ADDRESS INFORMATION

AddressInfo(
  intern 	192.168.1.19	192.168.0.0/16	00:50:ba:85:84:a9,
  extern	209.6.198.213	209.6.198.0/24	00:e0:98:09:ab:af,
  extern_next_hop				02:00:0a:11:22:1f,
  intern_server	192.168.1.10
);


// DEVICE SETUP

elementclass GatewayDevice {
  $trace |
  from::FromDump($trace, BIGMEM 2000000) 
	-> output;
  input -> tf::ThreadFanner; 
  tf[0] -> Discard;
  tf[1] -> to::Discard;
  tf[2] -> to2::Discard;
  tf[3] -> to3::Discard;
  tf[4] -> to4::Discard;

 // tf[5] -> to5::Discard;
 // tf[6] -> to6::Discard;
}

extern_dev :: GatewayDevice("/home/justine/mfiveseven.ingress.pcap");
intern_dev2 :: GatewayDevice("/home/justine/mfiveseven.egress.pcap");
intern_dev3 :: GatewayDevice("/home/justine/mfiveseven.egress.pcap");
intern_dev4 :: GatewayDevice("/home/justine/mfiveseven.egress.pcap");
//intern_dev5 :: GatewayDevice("/home/justine/mfiveseven.egress.pcap");
//intern_dev6 :: GatewayDevice("/home/justine/mfiveseven.egress.pcap");
intern_dev :: GatewayDevice("/home/justine/mfiveseven.egress.pcap");


ip_to_host :: EtherEncap(0x0800, 1:1:1:1:1:1, intern)
	-> Discard;


// ARP MACHINERY

extern_arp_class, intern_arp_class
	:: Classifier(12/0806 20/0001, 12/0806 20/0002, 12/0800, -);
intern_arpq :: ARPQuerier(intern);

extern_dev -> extern_arp_class;
extern_arp_class[0] -> ARPResponder(extern)	// ARP queries
	-> extern_dev;
extern_arp_class[1] -> Discard;			// ARP responses
extern_arp_class[3] -> Discard;

intern_dev -> intern_arp_class;
intern_dev2 -> intern_arp_class;
intern_dev3 -> intern_arp_class;
intern_dev4 -> intern_arp_class;
//intern_dev5 -> intern_arp_class;
//intern_dev6 -> intern_arp_class;


intern_arp_class[0] -> ARPResponder(intern)	// ARP queries
	-> intern_dev;
intern_arp_class[1] -> intern_arpr_t :: Tee;
	intern_arpr_t[0] -> Discard;
	intern_arpr_t[1] -> [1]intern_arpq;
intern_arp_class[3] -> Discard;


// REWRITERS

IPRewriterPatterns(to_world_pat extern 50000-65535 - -,
		to_server_pat intern 50000-65535 intern_server -);

rw :: ThreadSafeIPRewriter(// internal traffic to outside world
		 pattern to_world_pat 0 1,
     // external traffic redirected to 'intern_server'
     pattern to_server_pat 1 0,
     // internal traffic redirected to 'intern_server'
     pattern to_server_pat 1 1,
     // virtual wire to output 0 if no mapping
     pass 0,
     // virtual wire to output 2 if no mapping
     pass 2);

tcp_rw :: ThreadSafeTCPRewriter(// internal traffic to outside world
    pattern to_world_pat 0 1,
    // everything else is dropped
    drop);
Idle -> [0]tcp_rw;

// OUTPUT PATH

ip_to_extern :: GetIPAddress(16)
  -> CheckIPHeader
-> EtherEncap(0x0800, extern:eth, extern_next_hop:eth)
  -> extern_dev;
ip_to_intern :: GetIPAddress(16)
  -> CheckIPHeader
  -> [0]intern_arpq
  -> intern_dev;


  // to outside world or gateway from inside network
  rw[0] -> ip_to_extern_class :: IPClassifier(dst host intern, -);
  ip_to_extern_class[0] -> ip_to_host;
  ip_to_extern_class[1] -> ip_to_extern;
  // to server
  rw[1] -> ip_to_intern;
  // only accept packets from outside world to gateway
rw[2] -> IPClassifier(dst host extern)
  -> ip_to_host;

  // tcp_rw is used only for FTP control traffic
  tcp_rw[0] -> ip_to_extern;
  tcp_rw[1] -> ip_to_intern;


  // FILTER & REWRITE IP PACKETS FROM OUTSIDE

  ip_from_extern :: IPClassifier(dst host extern,
      -);
my_ip_from_extern :: IPClassifier(dst tcp ssh,
    dst tcp www or https,
    src tcp port ftp,
    tcp or udp,
    -);

extern_arp_class[2] -> Strip(14)
  -> CheckIPHeader
  -> ip_from_extern;
  ip_from_extern[0] -> my_ip_from_extern;
  my_ip_from_extern[0] -> [1]rw; // SSH traffic (rewrite to server)
  my_ip_from_extern[1] -> [1]rw; // HTTP(S) traffic (rewrite to server)
  my_ip_from_extern[2] -> [1]tcp_rw; // FTP control traffic, rewrite w/tcp_rw
  my_ip_from_extern[3] -> [4]rw; // other TCP or UDP traffic, rewrite or to gw
  my_ip_from_extern[4] -> Discard; // non TCP or UDP traffic is dropped
  ip_from_extern[1] -> Discard;	// stuff for other people


  // FILTER & REWRITE IP PACKETS FROM INSIDE

  ip_from_intern :: IPClassifier(dst host intern,
      dst net intern,
      dst tcp port ftp,
      -);
my_ip_from_intern :: IPClassifier(dst tcp ssh,
    dst tcp www or https,
    src or dst port dns,
    dst tcp port auth,
    tcp or udp,
    -);

intern_arp_class[2] -> Strip(14)
  -> CheckIPHeader
  -> ip_from_intern;
  ip_from_intern[0] -> my_ip_from_intern; // stuff for 10.0.0.1 from inside
  my_ip_from_intern[0] -> ip_to_host; // SSH traffic to gw
  my_ip_from_intern[1] -> [2]rw; // HTTP(S) traffic, redirect to server instead
  my_ip_from_intern[2] -> Discard;  // DNS (no DNS allowed yet)
  my_ip_from_intern[3] -> ip_to_host; // auth traffic, gw will reject it
  my_ip_from_intern[4] -> [3]rw; // other TCP or UDP traffic, send to linux
  // but pass it thru rw in case it is the
  // returning redirect HTTP traffic from server
  my_ip_from_intern[5] -> ip_to_host; // non TCP or UDP traffic, to linux
  ip_from_intern[1] -> ip_to_host; // other net 10 stuff, like broadcasts
ip_from_intern[2] -> Discard; 

  ip_from_intern[3] -> [0]rw;	// stuff for outside

  Idle -> intern_dev2;
  Idle -> intern_dev3;
  Idle -> intern_dev4;
  //Idle -> intern_dev5;
  //Idle -> intern_dev6;

  dm::DriverManager(
      set a 0,
      label x,
      //print "WAITING",
      wait 1s,

      set e0 $(extern_dev/to.count),
      set e1 $(extern_dev/to2.count),
      set e2 $(extern_dev/to3.count),
      set e3 $(extern_dev/to4.count),

   //   set e4 $(extern_dev/to5.count),
   //   set e5 $(extern_dev/to6.count),

      set b $(add $e0 $e1 $e2 $e3),
      print "egress done $(sub $b $a) packets in 1s",
      set a $b,

      goto x 1
      ); 



StaticThreadSched(dm 0, extern_dev/from 0, intern_dev/from 1, intern_dev2/from 2, intern_dev3/from 3, intern_dev4/from 4)

