#!/usr/bin/ruby
require 'trollop'
# Comparing the output for two pcap files -- did the NAT get properly mapped?

opts = Trollop.options do
	opt :master, "Master pcap file", :type => :string
  opt :replica, "Replica pcap file", :type => :string
  opt :exactmatch, "Whether to do exact match -- that is, packet by packet match -- or to allow some reordering."
end

p opts[:master]
p opts[:replica]

Trollop.die :master, "No master file?" if (!opts[:master] || !File.exist?(opts[:replica]))
Trollop.die :replica, "No replica file?" if (!opts[:replica] || !File.exist?(opts[:master]))

`tcpdump -nr #{opts[:master]} >mtxt`
`tcpdump -nr #{opts[:replica]} >rtxt`

themappings = {}

mf = File.open("mtxt")
timestep = 0
mf.each{|line|
  vals = line.chomp.split
  natip = vals[2]
  origip = vals[4]
  #puts "#{natip} #{origip}"
  themappings[origip] = {} if themappings[origip].nil?
  themappings[origip][timestep] = natip

  timestep += 1
}
mf.close

#puts themappings.inspect

rf = File.open("rtxt")
timestep = 0
rf.each{|line|
  vals = line.chomp.split
  natip = vals[2]
  origip = vals[4]

  val = themappings[origip]
  #  val = val[timestep] if !val.nil?
  puts "#{timestep} #{origip} #{natip} #{val.inspect}"
  timestep += 1
}

`rm mtxt`
`rm rtxt`
