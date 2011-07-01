#!/usr/bin/env python

import sys

#global_start_time = -1

# create tasks_map, tasklists
tasks_map = {}
tasklists = {}

loctree = None

###############################################################################

class TaskTrace:
  def __init__(self, mapline):
    tmp = mapline.strip().split()
    self.tid     = int(tmp[0])
    self.locvec  = tmp[1]
    self.name    = tmp[2]
    self.wid     = int(tmp[3])
    self.records = []
    self.total = -1
    self.ttbd = -1

  def __str__(self):
    return "Task %u @%u (%s %s) %.6f" % (self.tid, self.wid, self.locvec, self.name, self.total)

  def __repr__(self):
    return "t%u" % (self.tid)

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

    #self.total /= 1000000000.0
    #self.ttbd /= 1000000000.0

    # avg
    self.disp = len(self.records)
    self.avg = self.total / self.disp
    self.mtbd = self.ttbd / (self.disp-1) if self.disp > 1 else 0

###############################################################################


class TaskRecord:
  def __init__(self, line, parent):
    fields = line.split()
    pos = 0
    while (pos < len(fields)):
      if pos == 0:
        # end time
        self.end = float(fields[0]) # - global_start_time
        # check tid
        pos += 1; assert( parent.tid == int(fields[pos]))
        pass
      elif fields[pos] == "disp":
        # check disp
        pos += 1; assert( len(parent.records)+1 == int(fields[pos]) )
      elif fields[pos] == "st":
        pos += 1; self.st = fields[pos]
      elif fields[pos] == "et":
        pos += 1; self.et = float(fields[pos])/1000000000.0
      elif fields[pos] == "creat":
        pos += 1; self.creat = float(fields[pos])
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



class LocTree:
  class Node:
    def __init__(self, val):
      self.val = val
      self.children = dict()
      self.leaves = set()
      self.time = -1

    def __repr__(self):
      if self.val == (None,None):
        return "[ ROOT ] %.6f" % (self.time)
      else:
        if self.val[1]==-1:
          return "[ %c%c ] %.6f" % (self.val[0], '*', self.time)
        else: return "[ %c%u ] %.6f" % (self.val[0], self.val[1], self.time)

  # LocTree methods ######################

  def __init__(self):
    self.root = LocTree.Node((None,None))

  def _str2lbl(self, s):
    if len(s)>1:
      return (s[0], int(s[1:]))
    else: return (s[0], -1)

  def _getchild(self, node, val):
    if val not in node.children:
      n = LocTree.Node(val)
      node.children[val] = n
    else: n = node.children[val]
    return n

  def _extendpath(self, path, pos, node, task):
    if pos == len(path):
      node.leaves.add(task)
    else:
      (t,i) = path[pos]
      if i != -1:
        # insert intermediate topnode
        node = self._getchild(node, (t,-1))
      # the actual node
      newnode = self._getchild(node, (t,i))
      self._extendpath(path, pos+1, newnode, task)

  def extend(self, locvec, tasktrace):
    path = map( self._str2lbl, locvec.split(':')[1:])
    self._extendpath(path, 0, self.root, tasktrace)


  def _synth_from_children(self, node):
    node.time = 0.0
    # child nodes
    for v,ch in sorted(node.children.items()):
      self._synth_from_children(ch)
      node.time += ch.time
    # leaves
    for t in node.leaves:
      node.time += t.total

  def synthesize(self):
    self._synth_from_children(self.root)

  def _printnode(self, node, level):
    print level*"  ", node
    # print leaves
    for t in node.leaves:
      print (level+1)*"  ", t
    #print child nodes
    for v,ch in sorted(node.children.items()):
      self._printnode(ch, level+1)

  def doprint(self):
    self._printnode(self.root, 0)


###############################################################################

# main program entry point
if __name__=="__main__":

  if len(sys.argv) > 1: fname = sys.argv[1]
  else: fname = "mon_all.log"

  # fill tasks_map
  f = open("n00_tasks.map")
  try:
    loctree = LocTree()

    for mapline in f.readlines():
      task = TaskTrace(mapline)
      # tasks_map
      tasks_map[task.tid] = task
      # loc tree
      loctree.extend(task.locvec, task)
      # tasklists
      if not task.name in tasklists:
        tasklists[task.name] = [ task.tid ]
      else:
        tasklists[task.name].append( task.tid )
  finally: f.close()

  #print tasklists
  #for tid, obj in tasks_map.items(): print obj

  # just read global start time
  #f = open(fname)
  #try:
  #  global_start_time = float(f.readline().split()[0])
  #finally: f.close()
  #print "Global start time:", global_start_time



  # process each line of log
  f = open(fname)
  try:
    for line in f:
      tmp = line.split()

      if tmp[1] == "***": continue

      tid = int(tmp[1])
      trace = tasks_map[tid]
      trace.addRecord(line)
  finally: f.close()

  # compute summary for all task traces
  for task in tasks_map.values():
    if task.exists(): task.summary()

  # synthesize up total times in loctree
  loctree.synthesize()

  try:
    loctree.doprint()


    for taskname, lst in sorted(tasklists.items()):
      #if taskname[0] != "<":
        et_sum = 0
        print "====================================================="
        for tid in sorted(lst):
          trace = tasks_map[tid]
          if trace.exists():
            # trace.summary()
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

  except IOError, e:
    # prevent broken pipe errors
    pass

  exit(0)
