CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Iinclude
SRCS = src/instance.cpp src/solver_exact.cpp src/solver_baselines.cpp src/solver_proposed.cpp src/experiment_runner.cpp src/main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = task_offloading

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET) --demo

bench: $(TARGET)
	./$(TARGET) --all

.PHONY: all clean run bench

