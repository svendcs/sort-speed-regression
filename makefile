all: bin/faster bin/master
.PHONY: bin/faster bin/master
bin:
	mkdir bin
bin/faster: test.cpp bin
	g++ test.cpp  -I tpie-faster -Itpie-faster/build --std=c++11 -ltpie -Ltpie-faster/build/tpie -lboost_system -lboost_thread -lboost_regex -lboost_filesystem -o bin/faster
bin/master: test.cpp bin
	g++ test.cpp  -I tpie-master -Itpie-master/build --std=c++11 -ltpie -Ltpie-master/build/tpie -lboost_system -lboost_thread -lboost_regex -lboost_filesystem -o bin/master
