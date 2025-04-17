#Specify the cross-compiler 
CXX = aarch64-linux-gnu-g++

#Add path to lib dir and any other library your program will be using (e.g. -lcurl)
LIBS = -L/usr/lib/aarch64-linux-gnu -L/usr/lib/aarch64-linux-gnu/lib -lturbojpeg -static 

#Add path to include dir and header files belonging to your source code 
INC = -L/usr/include/aarch64-linux-gnu

server: streamer 
	$(CXX) -o server streamer.o $(LIBS)

streamer: streamer.cpp
	$(CXX) -Wall $(INC) -ljpeg -c streamer.cpp

clean: 
	rm -rf streamer.o server

