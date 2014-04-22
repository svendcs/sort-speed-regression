all: bin/faster bin/master bin/run_formation
.PHONY: bin/faster bin/master
bin:
	mkdir bin
bin/faster: sort_test.cpp bin
	g++ sort_test.cpp -DFASTER  -I tpie-faster -Itpie-faster/build --std=c++11 -ltpie -Ltpie-faster/build/tpie -lboost_system -lboost_thread -lboost_regex -lboost_filesystem -o bin/faster
bin/master: sort_test.cpp bin
	g++ sort_test.cpp -DMASTER -I tpie-master -Itpie-master/build --std=c++11 -ltpie -Ltpie-master/build/tpie -lboost_system -lboost_thread -lboost_regex -lboost_filesystem -o bin/master
bin/run_formation: run_formation_test.cpp bin
	g++ run_formation_test.cpp  -I tpie-faster -Itpie-faster/build --std=c++11 -ltpie -Ltpie-faster/build/tpie -lboost_system -lboost_thread -lboost_regex -lboost_filesystem -o bin/run_formation
