#!/usr/bin/ruby

require 'trollop'

opts = Trollop::options do
  #opt :name, "Monkey name", :type => :string        # string --name <s>, default nil
  opt :elts, "Number of elements in pipeline", :default => 8  # integer --num-limbs <i>, default to 4
  opt :locks, "Number of locks in pipeline", :default => 1
  opt :cores, "Number of cores", :default => 1
end
Trollop::die :locks, "must be less than or equal to --elts" if opts[:locks] > opts[:elts]



threadschedlist = ""

##Set up the pipeline
lines = []
opts[:locks].times{|e|
    opts[:cores].times{|c|
      lines << "m#{e}[#{c}] -> ue#{e}c#{c}::Unqueue2(BURST 0, QUIET 1) -> m#{e+1};"
      threadschedlist += "ue#{e}c#{c} #{c}, "
    }
    lines << "m#{e}::MultiThreadMultiQueue;"
}
str = ""
(opts[:elts] - opts[:locks]).times{|e|
  if(e == 0) then
    str += "m#{e + opts[:locks]}::PushNull ->"
  else
    str += " PushNull ->"
  end
}
str += "m#{opts[:elts]};" if(opts[:elts] > opts[:locks])
lines << str
lines << "m#{opts[:elts]}::ThreadFanner;"

puts lines.reverse

opts[:cores].times{|c|
  puts "i#{c}::InfiniteSource(LENGTH 64, BURST 64) -> m0;"
  puts "m#{opts[:elts]}[#{c}] -> d#{c}::Discard;"
}

##Create the DriverManager
puts
puts "DriverManager("
#Create the variables for historical output
opts[:cores].times{|c| puts "    set a#{c} 0,"}
puts "    label x, 
    wait 1s,"
#Get the current outputs and print them
opts[:cores].times{|c| puts "
    set b#{c} $(d#{c}.count),
    print \"core#{c} done $(sub $b#{c} $a#{c}) packets in 1s\",
    set a#{c} $b#{c},"}
#Finish the script
puts "
    goto x 1
);
"

#Create the threadschedule
opts[:cores].times{|c|
  if(c != opts[:cores] - 1) then
    threadschedlist += "i#{c} #{c}, d#{c} #{c}, "
  else
    threadschedlist += "i#{c} #{c}, d#{c} #{c}"
  end
}

puts "StaticThreadSched(#{threadschedlist})"
