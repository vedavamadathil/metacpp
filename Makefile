demo: metacpp_demo.cpp metacpp.hpp lisp.hpp
	g++ -std=c++20 metacpp_demo.cpp -o demo

run: demo
	./demo
