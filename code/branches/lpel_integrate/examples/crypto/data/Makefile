CC        = mpicc
CCFLAGS   = -Wall
LIBS      = -lcrypt 

# = = = = = = = = = = = = = = = = = = = =
TARGET    = pgen
# = = = = = = = = = = = = = = = = = = = =


all:   
	$(CC) $(CCFLAGS) -c *.c
	$(CC) -o $(TARGET) *.o $(LIBS)
clean:
	rm -f *.o *~ $(TARGET)
