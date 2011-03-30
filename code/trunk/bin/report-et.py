#!/usr/bin/env python

import sys

global_start_time = -1

# create tasks_map, tasklists
tasks_map = {}
tasklists = {}


###############################################################################

class TaskTrace:
  def __init__(self, tid, name):
    self.tid = tid
    self.name = name
    self.records = []

  def __str__(self):
    return "Task %u (%s)" % (self.tid, self.name)

  def exists(self):
    return len(self.records) > 0

  def addRecord(self, line):
    rec = TaskRecord( line, self)
    # add to record list
    self.records.append( rec)
    # update globals
    if rec.st == "Z":
      self.creat = rec.creat
      self.destr = rec.end

  def summary(self):
    self.total = 0
    self.ttbd = 0
    # iterate through records
    for rec in self.records:
      self.total += rec.et
      self.ttbd  += rec.wt

    self.total /= 1000000000.0
    self.ttbd /= 1000000000.0

    # avg
    self.disp = len(self.records)
    self.avg = self.total / self.disp
    if self.disp > 1: self.mtbd = self.ttbd / (self.disp-1)
    else: self.mtbd = self.ttbd

###############################################################################


class TaskRecord:
  def __init__(self, line, parent):
    fields = line.split()
    pos = 0
    while (pos < len(fields)):
      if pos == 0:
        # end time
        self.end = int(fields[0]) - global_start_time
        pass
      elif fields[pos] == "tid":
        # check tid
        pos += 1; assert( parent.tid == int(fields[pos]))
      elif fields[pos] == "disp":
        # check disp
        pos += 1; assert( len(parent.records)+1 == int(fields[pos]) )
      elif fields[pos] == "st":
        pos += 1; self.st = fields[pos]
      elif fields[pos] == "et":
        pos += 1; self.et = int(fields[pos])
      elif fields[pos] == "creat":
        pos += 1; self.creat = int(fields[pos])
      elif fields[pos][0] == "[":
        # stream info
        pass

      # next field
      pos += 1

    # calc start time
    self.start = self.end - self.et
    # calc wait time
    if len(parent.records) > 0:
      self.wt = self.start - parent.records[-1].end
    else: self.wt = 0


###############################################################################

if len(sys.argv) > 1: fname = sys.argv[1]
else: fname = "mon_all.log"

# fill tasks_map
f = open("tasks.map")
try:
  for line in f.readlines():
    tmp = line.split()
    tid = int(tmp[0])
    taskname = tmp[1].strip()
    # tasks_map
    tasks_map[tid] = TaskTrace(tid, taskname)
    # tasklists
    if not taskname in tasklists:
      tasklists[taskname] = [ tid ]
    else:
      tasklists[taskname].append( tid )
finally: f.close()

#print tasklists

#for tid, obj in tasks_map.items(): print obj


# just read global start time
f = open(fname)
try:
  global_start_time = int(f.readline().split()[0])
finally: f.close()
#print "Global start time:", global_start_time



# process each line of log
f = open(fname)
try:
  for line in f:
    tmp = line.split()

    if tmp[1] == "***": continue

    tid = int(tmp[2])
    trace = tasks_map[tid]
    trace.addRecord(line)
finally: f.close()


for taskname, lst in tasklists.items():
  #if taskname[0] != "<":
    et_sum = 0
    print "====================================================="
    for tid in sorted(lst):
      trace = tasks_map[tid]
      if trace.exists():
        trace.summary()
        print trace, "disp %u total %.6f, avg %.6f, ttbd %.6f, mtbd %.6f" \
              % (trace.disp, trace.total, trace.avg, trace.ttbd, trace.mtbd)
        et_sum += trace.total
    et_avg = et_sum / len(lst)
    print "*** %s: total %.6f, avg %.6f" % (taskname, et_sum, et_avg)

exit(0)

for box, et in times.items():
  secs = et / 1000000000.0
  print "%s %f %.6f%c" % (box, secs, ( 100.0*et / et_sum), '%')

print "=========="
print "total ", et_sum / 1000000000.0

exit(0)
