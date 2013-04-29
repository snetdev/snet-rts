#!/usr/bin/python

import sys
import numpy
import random

def array_to_str(N, arr):
	l = arr.reshape(N*N).tolist()[0]
	s = '(double[' + str(N * N) + '])'  
	s += ','.join(map(str, l))
	return s

# generate XML with input
def generate_snet_in(A, N, BS, name):
	s  = '<?xml version="1.0" encoding="ISO-8859-1" ?>\n'
	s += '<record xmlns="snet-home.org" type="data" mode="textual" interface="C4SNet">\n'
	s += '\t<field label="Ain">' + array_to_str(N, A) + '</field>\n'
	s += '\t<tag label="dim">' + str(N) + '</tag>\n'
	s += '\t<tag label="bs">' + str(BS) + '</tag>\n'
	s += '</record>\n'
	s += '<record type="terminate" />'
	f = open(name + '_snet_in.xml', 'w')
	f.write(s)
	f.close()

# generate XML with output
def generate_snet_out(L, N, BS, name):
	s  = '<?xml version="1.0" ?>\n'
	s += '<record xmlns="snet-home.org" type="data" mode="textual" >\n'
	s += '\t<field label="Lresult" interface="C4SNet">' + array_to_str(N, L) + '</field>\n'
	# s += '\t<tag label="bs">' + str(BS) + '</tag>\n'
	# s += '\t<tag label="p">' + str(N / BS) + '</tag>\n'
	s += '</record>\n'
	s += '<?xml version="1.0" ?>\n'
	s += '<record type="terminate" />'
	f = open(name + '_snet_out.xml', 'w')
	f.write(s)
	f.close()

# Parse input arguments
if len(sys.argv) != 4:
	print 'Incorrect number of arguments, expected N BS test_name'
	sys.exit(1)
try:
	N = int(sys.argv[1])
	if N <= 0:
		raise ValueError('N is not a positive integer')
except ValueError:
	print 'Argument N is invalid. A positive integer expected'
	sys.exit(1)
try:
	BS = int(sys.argv[2])
	if BS <= 0:
		raise ValueError('N is not a positive integer')
except ValueError:
	print 'Argument N is invalid. A positive integer expected'
	sys.exit(1)
if BS > N:
	print 'The size of a block BS must be less or equal than dimension N'
	sys.exit(1)

Larr = numpy.zeros(shape=(N,N))
for i in range(N):
	for j in range(i+1):
		if i == j:
			Larr[i][j] = random.randrange(999) + 1
		else:
			Larr[i][j] = random.randrange(2000) - 1000
# output matrix
L = numpy.matrix(Larr)
# input matrix
A = L * L.T

generate_snet_in(A, N, BS, sys.argv[3])
generate_snet_out(L, N, BS, sys.argv[3])
