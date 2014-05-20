// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; c-file-style: "stroustrup"; -*-// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2009, The TPIE development team
// 
// This file is part of TPIE.
// 
// TPIE is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
// 
// TPIE is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with TPIE.  If not, see <http://www.gnu.org/licenses/>
#include <iostream>
#include <tpie/tpie.h>
#include <tpie/parallel_sort.h>
#include <tpie/pipelining/merge_sorter.h>

#include <boost/filesystem/operations.hpp>

#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "stat_reporter.hpp"

using namespace tpie;


struct Timer {
	Timer() {
		start = boost::posix_time::microsec_clock::local_time();
	}

	memory_size_type elapsed() {
		boost::posix_time::ptime current = boost::posix_time::microsec_clock::local_time();
		return (current-start).total_milliseconds();
	}

	void reset() {
		start = boost::posix_time::microsec_clock::local_time();
	}
private:
	boost::posix_time::ptime start;
};

void usage() {
	std::cout << "Parameters: [times] [data/MiB] [memory/MiB]" << std::endl;
}

struct LargeWithSmallComp {
	memory_size_type s1;
	memory_size_type s2;
	memory_size_type s3;
	memory_size_type s4;
	memory_size_type s5;
	memory_size_type s6;
	memory_size_type s7;
	memory_size_type s8;
	memory_size_type s9;
	memory_size_type s10;

	LargeWithSmallComp() : s1(0), s2(0), s3(0), s4(0), s5(0), s6(0), s7(0), s8(0), s9(0) {}
};

class pred {
public:
	typedef const LargeWithSmallComp& item_type;
	typedef item_type first_argument_type;
	typedef item_type second_argument_type;
	typedef bool result_type;

	bool operator()(item_type a, item_type b) {
		return a.s1 < b.s1;
	}
};

int main(int argc, char **argv) {
	///////////////////////////////////////////////////////////////////////////////
	/// Default values
	///////////////////////////////////////////////////////////////////////////////
	size_t data = 10;
	size_t memory = 50;

	///////////////////////////////////////////////////////////////////////////////
	/// Read args
	///////////////////////////////////////////////////////////////////////////////
	if (argc > 1) {
		std::stringstream(argv[1]) >> data;
		if (!data) {
			usage();
			return EXIT_FAILURE;
		}
	}
	if (argc > 2) {
		std::stringstream(argv[2]) >> memory;
		if (!memory) {
			usage();
			return EXIT_FAILURE;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	/// TPIE init
	///////////////////////////////////////////////////////////////////////////////
	tpie::tpie_init();
	//tpie::get_memory_manager().set_limit(memory*1024*1024);
	std::cout << "Data: " << data << "MiB" << std::endl;
	std::cout << "Memory: " << memory << "MiB" << std::endl;

	///////////////////////////////////////////////////////////////////////////////
	/// Perform test
	///////////////////////////////////////////////////////////////////////////////

	memory_size_type count = data * 1024 * 1024 / sizeof(LargeWithSmallComp);

	merge_sorter<LargeWithSmallComp, false, pred> merge_sorter;
	Timer timer;

	// Phase 0 set parameters
	{
		//merge_sorter.set_available_memory(get_memory_manager().available());
		//merge_sorter.set_parameters(get_block_size() / sizeof(memory_size_type), 32);
		merge_sorter.set_available_memory(memory * 1024 * 1024);
	}
	memory_size_type phase0 = timer.elapsed();
	std::cout << "Phase 0: " << phase0 << std::endl;

	StatReporter reporter("results.tab");
	reporter.run();
	LargeWithSmallComp e;

	timer.reset();
	{
		merge_sorter.begin();
		for(memory_size_type i = 0; i < count; ++i) {
			e.s1 = i;
			merge_sorter.push(e);
		}
		merge_sorter.end();
	}
	memory_size_type phase1 = timer.elapsed();
	std::cout << "Phase 1: " << phase1 << std::endl;


	// Phase 2 - Perform merges
	dummy_progress_indicator pi;
	timer.reset();
	{
		merge_sorter.calc(pi);
	}
	memory_size_type phase2 = timer.elapsed();
	std::cout << "Phase 2: " << phase2 << std::endl;

	// Phase 3 - Pull
	memory_size_type l = 0;
	memory_size_type hest = 0;
	timer.reset();
	{
		//log_info() << "<pull_begin>" << std::endl;
		#ifdef FASTER // the interface is slightly changed
		merge_sorter.pull_begin();
		#endif
		//log_info() << "</pull_begin>" << std::endl;

		//log_info() << "<pull>" << std::endl;
		for(memory_size_type i = 0; i < count; ++i) {
			LargeWithSmallComp e = merge_sorter.pull();
			//tp_assert(e.s1 >= l, "Elements were not sorted");
			hest ^= e.s1;
			l = e.s1;
		}
		//log_info() << "</pull>" << std::endl;

		/*log_info() << "<pull_end>" << std::endl;
		#ifdef FASTER
		merge_sorter.pull_end();
		#endif
		log_info() << "</pull_end>" << std::endl;*/
	}

	if(hest == 42) {
		std::cout << "Det var dog underligt." << std::endl; // science
	}
	memory_size_type phase3 = timer.elapsed();
	reporter.stop();

	std::cout << "Phase 3: " << phase3 << std::endl;
	std::cout << "Total: " << phase0+phase1+phase2+phase3 << std::endl;

	tpie_finish();
	return 0;
}