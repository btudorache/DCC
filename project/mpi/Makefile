CC = mpicc
CFLAGS = -Wall -O2
TARGET = alltoall
SRCS = alltoall.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean

run:
	mpirun -np 3 --mca plm_rsh_agent ssh --mca plm_rsh_args "-o StrictHostKeyChecking=no" --hostfile hostfile ./$(TARGET)