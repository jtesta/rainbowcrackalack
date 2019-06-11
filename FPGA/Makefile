CPP=g++
COMPILE_OPTIONS=-Wall -O3 -std=c++11 -DNOT_AWS_FPGA
LINK_OPTIONS=-lOpenCL

.PHONY: clean

all:	crackalack_fpga_gen

%.o: %.cpp
	$(CPP) $(COMPILE_OPTIONS) -o $@ -c $<

crackalack_fpga_gen: crackalack_fpga_gen.o
	$(CPP) -o crackalack_fpga_gen crackalack_fpga_gen.o $(LINK_OPTIONS)
clean:
	rm -f *~ *.o crackalack_fpga_gen
