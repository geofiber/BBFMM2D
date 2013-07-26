CC=g++
CFLAGS=-c -Wall -DNDEBUG -O3 -ffast-math -ffinite-math-only -I header/ -I ./../../
LDFLAGS=

SOURCES=src/H2_2D_tree.cpp src/H2_2D_node.cpp src/read_location_H.cpp src/kernel_base.cpp src/kernel_types.cpp

SOURCESA=examples/H2_2D_MVP_input_from_file_standard_kernel.cpp
SOURCESB=examples/H2_2D_MVP_get_matrix_through_routine_standard_kernel.cpp 
SOURCESC=examples/H2_2D_MVP_input_from_file_mykernel.cpp
SOURCESD=examples/H2_2D_MVP_get_matrix_through_routine_mykernel.cpp

OBJECTSA=$(SOURCES:.cpp=.o) $(SOURCESA:.cpp=.o) 
OBJECTSB=$(SOURCES:.cpp=.o) $(SOURCESB:.cpp=.o) 
OBJECTSC=$(SOURCES:.cpp=.o) $(SOURCESC:.cpp=.o)
OBJECTSD=$(SOURCES:.cpp=.o) $(SOURCESD:.cpp=.o)


EXECUTABLEA=./exec/H2_2D_MVP_input_from_file_standard_kernel
EXECUTABLEB=./exec/H2_2D_MVP_get_matrix_through_routine_standard_kernel
EXECUTABLEC=./exec/H2_2D_MVP_input_from_file_mykernel
EXECUTABLED=./exec/H2_2D_MVP_get_matrix_through_routine_mykernel


input_from_file_standard_kernel: $(SOURCES) $(SOURCESA) $(EXECUTABLEA)
	
$(EXECUTABLEA): $(OBJECTSA)
	$(CC) $(LDFLAGS) $(KERNEL) $(INDEX) $(OBJECTSA) -o $@


get_matrix_through_routine_standard_kernel: $(SOURCES) $(SOURCESB) $(EXECUTABLEB)
	
$(EXECUTABLEB): $(OBJECTSB)
	$(CC) $(LDFLAGS) $(KERNEL) $(INDEX) $(OBJECTSB) -o $@


input_from_file_mykernel: $(SOURCES) $(SOURCESC) $(EXECUTABLEC)
	
$(EXECUTABLEC): $(OBJECTSC)
	$(CC) $(LDFLAGS) $(KERNEL) $(INDEX) $(OBJECTSC) -o $@


get_matrix_through_routine_mykernel: $(SOURCES) $(SOURCESD) $(EXECUTABLED)
	
$(EXECUTABLED): $(OBJECTSD)
	$(CC) $(LDFLAGS) $(KERNEL) $(INDEX) $(OBJECTSD) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(KERNEL) $(INDEX) $< -o $@

clean:
	rm -rf *.out examples/*.o src/*.o exec/*

tar:
	tar -zcvf H2_2D_MVP.tar.gz ./examples/H2_2D_MVP_input_from_file_standard_kernel.cpp ./examples/H2_2D_MVP_get_matrix_through_routine_standard_kernel.cpp ./examples/H2_2D_MVP_input_from_file_mykernel.cpp ./examples/H2_2D_MVP_get_matrix_through_routine_mykernel.cpp ./src/H2_2D_tree.cpp ./src/H2_2D_node.cpp src/read_inputfile.cpp ./header/read_inputfile.hpp ./header/H2_2D_tree.hpp ./header/H2_2D_node.hpp Makefile

