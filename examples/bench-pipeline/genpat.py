#! /usr/bin/env python

import sys

params = sys.argv[1]

insidet, pipe_len, par_width = params.split('_')
pipe_len = int(pipe_len)
par_width = int(par_width)

if par_width == 0:
    gen = "gent"
else:
    # define the generator box
    gen = "genxt"
    print "box genxt ((<a>,<b>,<c>,<p>) -> %s);" % \
        "|".join("(<a>,<b>,<c>,<x%d>)" % i for i in range(par_width))

print "} connect "

if insidet == "pt":
    inside = "addt"
elif insidet == "bn":
    inside = "(addt!<c>)"
elif insidet == "bd":
    inside = "(addt!!<c>)"
elif insidet == "sn":
    inside = "([{<c>}->{<c=c>,<i=c>}] .. (addt .. [{<i>} -> if <i==0> then {<i=i>,<stop>} else {<i=i-1>}]) * {<stop>} .. [{<stop>}->{}] )"
elif insidet == "sd":
    inside = "([{<c>}->{<c=c>,<i=c>}] .. (addt .. [{<i>} -> if <i==0> then {<i=i>,<stop>} else {<i=i-1>}]) ** {<stop>} .. [{<stop>}->{}] )"

if par_width > 0:
    def geninside(i):
        if par_width > 1:
            return "[{<x%d>}->{<x%d=x%d>}] .. %s" % (i,i,i, inside)
        else:
            return inside

    inside = "(%s)" % "|\n     ".join("(%s)" % geninside(i) for i in range(par_width))
    
items = ["[{} -> {<p=%d>}]" % par_width, 
         "f2t", gen] + [inside for j in range(pipe_len)] + ["dropt"]

print "\n .. ".join(items), ";"
