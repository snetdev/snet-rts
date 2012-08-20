#! /usr/bin/env python

import sys
import os.path

cfgfile = sys.argv[1]
instbase = sys.argv[2]

configs = []
for line in file(cfgfile):
    line = line.strip()
    if line.startswith('#') or len(line) == 0:
        continue
    configs.append(line.split(' ', 4))

paramlist = [
        ('pt', 1,  1000000, 1),
        ('pt', 5,   200000, 1),
        ('pt', 10,  100000, 1),
        ('pt', 50,   20000, 1),
        ('pt', 100,  10000, 1),
        ('pt', 1,    10000, 10000000),
        ('pt', 5,     2000, 10000000),
        ('pt', 10,    1000, 10000000),
        ('pt', 50,     200, 10000000),
        ('pt', 100,    100, 10000000),
        ]

def gencmds():
    print "#! /bin/bash"
    for bparams in paramlist:
        params = '%s-%d-1-%d-%d-1' % bparams

        for cfg in configs:
            if len(cfg) == 4:
                cfg.append('')
            tag, desc, backend, inst, flags = cfg

            for fuzzy in [0, 100]:
                if bparams[3] == 1 and fuzzy == 100:
                    continue
                print "( eval $(%s); WORKLOAD_FUZZINESS=%d make result-%s-{1,2,3,4,5,6,8,10,12,14,16}-%s-nodist-%s-r%d TAG=-%s-r%d BENCH_EXTRA_ARGS='%s' BENCH_RUNS=3 BENCH_IGNORE_ERRORS=1 )" % \
                      (os.path.join(instbase, inst), fuzzy, params, backend, tag, fuzzy, tag, fuzzy, flags)

def gengp(combifile):
    dcfg = {}
    for cfg in configs:
        if len(cfg) == 4:
            cfg.append('')
        tag, desc, backend, inst, flags = cfg
        dcfg[tag] = (desc, backend)

    combis = []
    for l in file(combifile):
        l = l.strip()
        if l:
            items = l.split(' ')
            combis.append([x.strip() for x in items if x in dcfg])

    somedata = {}
    for bparams in paramlist:
        for fuzzy in [0, 100]:
            if bparams[3] == 1 and fuzzy == 100:
                continue
            params = '%s-%d-1-%d-%d-1' % bparams

            filebase = '%s-r%d' % (params, fuzzy)
            
            dfilename = '%s.dat' % filebase
            print "Generating %s..." % dfilename
            dfile = file(dfilename, 'w')
            print >>dfile, "# %s\n# ncores %s" % (dfilename, ' '.join(dcfg.keys()))

            minv = 1000
            maxv = 0
            somedata[tag]  = False
            for p in [1,2,3,4,5,6,8,10,12,14,16]:
                print >>dfile, p,

                for tag in dcfg.keys():
                    backend = dcfg[tag][1]

                    rfilename = 'result-%s-%d-%s-nodist-%s-r%d' % (params, p, backend, tag, fuzzy)
                    vn = []
                    try:
                        with file(rfilename) as rfile:
                            for l in rfile:
                                if l.startswith('real'):
                                    v = float(l[4:].strip())
                                    #if v > 600:
                                    #    pass
                                    vn.append(v)
                    except:
                        pass
                    if len(vn) == 0:
                        print >>dfile, '-',
                    else:
                        print >>dfile, min(vn),
                        minv = min(minv, min(vn))
                        maxv = max(maxv, min(vn))
                        somedata[tag] = True
                    
                print >>dfile

            dfile.close()

            gfilename = '%s.gp' % filebase
            print "Generating %s..." % gfilename
            gfile = file(gfilename, 'w')

            print >>gfile, """set terminal pdf size 20cm,20cm """
            print >>gfile, """set style data linespoints """
            print >>gfile, """set title "Net=%s length=%d nrecs=%d wload=%d fuzzy=%d" """ % (bparams+(fuzzy,))
            print >>gfile, """set xrange [0:17] \n set key outside bottom"""

            for gn, taglist in enumerate(combis):
                nodata = True
                for t in taglist:
                    if somedata[t]:
                        nodata = False
                        break
                if nodata:
                    continue

                print >>gfile, """ set output "%s-%d.pdf" \n unset logscale y \n set yrange [0:]  """ % (filebase, gn)
                print >>gfile, """plot """,
                first = True
                for i,tag in enumerate(taglist):
                    desc, backend = dcfg[tag]

                    if not first:
                        print >>gfile, ", \\"
                    else:
                        first = False

                    print >>gfile, """ "%s" using 1:%d """ % (dfilename, i+2),
                    print >>gfile, """ title "%s" """ % desc,

                print >>gfile
                print >>gfile, """set output "%s-%d-log.pdf" \n set yrange [%f:%f] \n set logscale y \n replot""" % (filebase, gn, minv, maxv)
            
            
            gfile.close()
            
            
if __name__ == "__main__":
    what = sys.argv[3]
    if what == "c":
        gencmds()
    elif what == "g":
        gengp(sys.argv[4])
