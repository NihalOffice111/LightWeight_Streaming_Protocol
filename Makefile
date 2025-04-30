# #Specify the cross-compiler 
# CXX = aarch64-linux-gnu-g++
# CXXFLAGS = -O2 -Wall


# #Add path to lib dir and any other library your program will be using (e.g. -lcurl)
# LIBS = -L/usr/lib/aarch64-linux-gnu -L/usr/lib/aarch64-linux-gnu/lib -lturbojpeg -static 

# #Add path to include dir and header files belonging to your source code 
# INC = -L/usr/include/aarch64-linux-gnu

# server: streamer 
# 	$(CXX) -o server streamer.o $(LIBS)

# streamer: streamer.cpp
# 	$(CXX) -Wall $(INC) -ljpeg -c streamer.cpp

# clean: 
# 	rm -rf streamer.o server


# Makefile for ARM64 Cross Compilation

# Toolchain
CXX = aarch64-linux-gnu-g++
CXXFLAGS = -O2 -Wall

# 🧱 Source and output
SRC = streamer.cpp
TARGET = server

# 📚 Link against required libraries
LDLIBS = -lturbojpeg -lopus -lasound -lpthread

# 🎯 Default build rule
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# 🧹 Clean up build
clean:
	rm -f $(TARGET)
