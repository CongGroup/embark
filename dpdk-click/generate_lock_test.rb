#!/usr/bin/ruby

require 'trollop'

opts = Trollop::options do
  #opt :name, "Monkey name", :type => :string        # string --name <s>, default nil
  opt :elts, "Number of elements in pipeline", :default => 8  # integer --num-limbs <i>, default to 4
  opt :locks, "Number of locks in pipeline", :default => 1
  opt :cores, "Number of cores", :default => 1
end
Trollop::die :locks, "must be less than or equal to --elts" if opts[:locks] > opts[:elts]




##Set up the pipeline
nullelts = ""
opts[:elts].times{|e|
  if(e < opts[:locks]) then
    nullelts += "BatchLockingPushNull -> "
  else
    nullelts += "PushNull -> " 
  end
}

puts "pipeline::#{nullelts}tf::ThreadFanner;"

opts[:cores].times{|c|
  puts "i#{c}::InfiniteSource(LENGTH 64, BURST 64) -> pipeline;"
  puts "tf[#{c}] -> d#{c}::Discard;"
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
threadschedlist = nil
opts[:cores].times{|c|
  if(threadschedlist.nil?)
    threadschedlist = "i0 0, d0 0" 
  else
    threadschedlist += ", i#{c} #{c}, d#{c} #{c}"
  end
}

puts "StaticThreadSched(#{threadschedlist})"
