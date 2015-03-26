#!/usr/bin/env python
import sys

if len(sys.argv) != 3:
    print "usage: %s <filename> <filename>" % sys.argv[0]
    sys.exit(1)

f1 = open(sys.argv[1])
lines1 = f1.readlines()

f2 = open(sys.argv[2])
lines2 = f2.readlines()

m = len(lines1)
n = len(lines2)

i = 0
j = 0
time = 0.0
p1 = '0.0'
p2 = '0.0'

while i < m and j < n:
    t1, p1_new = lines1[i].strip().split(', ')
    t2, p2_new = lines2[j].strip().split(', ')
    if float(t1) < float(t2):
        p1 = p1_new
        print "%s, %s, %s" % (t1, p1, p2)
        i += 1
    else:
        p2 = p2_new
        print "%s, %s, %s" % (t2, p1, p2)
        j += 1

while i < m:
    t1, p1 = lines1[i].strip().split(', ')
    print "%s, %s, %s" % (t1, p1, p2)
    i += 1

while j < n:
    t2, p2 = lines2[j].strip().split(', ')
    print "%s, %s, %s" % (t1, p1, p2)
    j += 1

