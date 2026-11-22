CXX = clang++
CXXFLAGS = -Wall -Wextra -ggdb -std=c++20 -Wno-missing-designated-field-initializers

main: main.cpp
	$(CXX) $(CXXFLAGS) -o main main.cpp
