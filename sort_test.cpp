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

int main(int argc, char **argv) {
	///////////////////////////////////////////////////////////////////////////////
	/// Default values
	///////////////////////////////////////////////////////////////////////////////
	size_t times = 10;
	size_t data = 10;
	size_t memory = 50;
			
	///////////////////////////////////////////////////////////////////////////////
	/// Read args
	///////////////////////////////////////////////////////////////////////////////
	if (argc > 1) {
		if (std::string(argv[1]) == "0") {
			times = 0;
		} else {
			std::stringstream(argv[1]) >> times;
			if (!times) {
				usage();
				return EXIT_FAILURE;
			}
		}
	}
	if (argc > 2) {
		std::stringstream(argv[2]) >> data;
		if (!data) {
			usage();
			return EXIT_FAILURE;
		}
	}
	if (argc > 3) {
		std::stringstream(argv[3]) >> memory;
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
	std::cout << "Times: " << times << std::endl;
	std::cout << "Data: " << data << "MiB" << std::endl;
	std::cout << "Memory: " << memory << "MiB" << std::endl;

	///////////////////////////////////////////////////////////////////////////////
	/// Perform test
	///////////////////////////////////////////////////////////////////////////////
	
	memory_size_type count = data * 1024 * 1024 / sizeof(memory_size_type);

	std::cout << "run" << std::setw(15) << "phase 0" << std::setw(15) << "phase 1" << std::setw(15) << "phase 2" << std::setw(15) << "phase 3"  << std::setw(15) << "total" << std::endl;
	for(memory_size_type i = 0; i < times; ++i) {
		std::cout << i << std::setw(15) << std::flush;
		merge_sorter<memory_size_type, false> merge_sorter;
		Timer timer;

		// Phase 0 set parameters
		{
			//merge_sorter.set_available_memory(get_memory_manager().available());
			//merge_sorter.set_parameters(get_block_size() / sizeof(memory_size_type), 32);
			merge_sorter.set_available_memory(memory * 1024 * 1024);
		}
		memory_size_type phase0 = timer.elapsed();
		std::cout << phase0 << std::setw(15) << std::flush;

		// Phase 1 - Push elements
		boost::mt19937 rng(42);
		boost::uniform_int<memory_size_type> dist(0, std::numeric_limits<memory_size_type>::max());
		boost::variate_generator<boost::mt19937&, boost::uniform_int<memory_size_type> > generator(rng, dist);

		timer.reset();
		{
			merge_sorter.begin();
			for(memory_size_type i = 0; i < count; ++i) {
				//merge_sorter.push(generator());
				merge_sorter.push(count - i);
			}
			merge_sorter.end();
		}
		memory_size_type phase1 = timer.elapsed();
		std::cout << phase1 << std::setw(15) << std::flush;


		// Phase 2 - Perform merges
		dummy_progress_indicator pi;
		timer.reset();
		{
			merge_sorter.calc(pi);
		}
		memory_size_type phase2 = timer.elapsed();
		std::cout << phase2 << std::setw(15) << std::flush;


		// Phase 3 - Pull
		memory_size_type l = 0;
		memory_size_type hest = 0;
		timer.reset();
		{
			#ifdef FASTER
			merge_sorter.pull_begin();
			#endif

			for(memory_size_type i = 0; i < count; ++i) {
				memory_size_type e = merge_sorter.pull();
				tp_assert(e >= l, "Elements were not sorted");
				hest ^= e;
				l = e;
			}

			#ifdef FASTER
			merge_sorter.pull_end();
			#endif
		}

		if(hest == 42) {
			std::cout << "Det var dog underligt." << std::endl; // science
		}
		memory_size_type phase3 = timer.elapsed();
		std::cout << phase3 << std::setw(15) << std::flush;
		std::cout << phase0+phase1+phase2+phase3 << std::endl;
	}


	tpie_finish();
	return 0;
}
