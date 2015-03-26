#!/usr/bin/env python
import sys

if len(sys.argv) != 2:
    print "usage: %s <filename>" % sys.argv[0]
    sys.exit(1)

f = open(sys.argv[1])

data = []

for line in f:
    sp = line.split()
    if len(sp) == 2: 
        data.append(float(sp[1])/1000)

data.sort()
n = len(data)

share = [0.0] * n;
share[0] = data[0]

for i in xrange(1,n):
    share[i] = share[i-1] + data[i]

print "%f, %f" % (0, 0.0)

for i in xrange(n):
    print "%f, %f" % (data[i], share[i]/share[n-1])
