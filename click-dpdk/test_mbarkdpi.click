InfiniteSource(LENGTH 500)
->  UDPIPEncap(1.0.0.1, 1234, 2.0.0.2, 1234)
->  dpi::MBArkDPI[0]
//->  Print
->  d::Discard;

dpi[1]=>d2::Discard;

dm::DriverManager(
  set a 0,
  label x,
  wait 1s,
  set b $(d.count),
  set c $(sub $b $a),
  print "done $c packets in 1s",
  set a $b,
  goto x 1
); 


