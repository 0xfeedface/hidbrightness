LDFLAGS=-lhidapi-hidraw
CXXFLAGS=-std=c++2b -Werror -Wall

hidbrightness: main.cc
	clang++ -O3 $(CXXFLAGS) -o hidbrightness main.cc $(LDFLAGS)

debug: main.cc
	clang++ -g -glldb -Og $(CXXFLAGS) -o hidbrightness main.cc $(LDFLAGS)

sanitize: main.cc
	clang++ -g -fno-omit-frame-pointer -fsanitize=address $(CXXFLAGS) -o hidbrightness main.cc $(LDFLAGS)
