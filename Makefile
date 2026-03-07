CXX = g++
CXXFLAGS = -std=c++17 -Wall -I utilities/include
LDFLAGS = -framework OpenGL -lglfw -lGLEW

TARGET = campus
SRCS = $(wildcard src/*.cpp) utilities/InitShader.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
