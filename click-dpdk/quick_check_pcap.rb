#!/usr/bin/ruby
require 'trollop'

opts = Trollop.options do
  opt :pcap, "PCAP file", :type => :string
end

Trollop.die :pcap, "No pcap file?" if (!opts[:pcap] || !File.exist?(opts[:pcap]))

puts "wc -l #{`tcpdump -nr #{opts[:pcap]} | wc -l`}"

puts "SWAPS"
s=`tcpdump -nr #{opts[:pcap]} | cut -d " " -f5 | cut -d "." -f1 | uniq -c | sed 's/^ *//' | cut -d " " -f1 | ~/tools/summarize_list_of_numbers.rb`
puts s
