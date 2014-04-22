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

int main(int argc, char **argv) {
	///////////////////////////////////////////////////////////////////////////////
	/// TPIE init
	///////////////////////////////////////////////////////////////////////////////
	tpie::tpie_init();
	//tpie::get_memory_manager().set_limit(memory*1024*1024);

	///////////////////////////////////////////////////////////////////////////////
	/// Perform test
	///////////////////////////////////////////////////////////////////////////////
	
	memory_size_type test_sizes[] = {10000, 20000};
	memory_size_type n = 2;

	std::cout << "test size" << std::setw(15) << "run 1" << std::setw(15) << "run 2" << std::setw(15) << "run 3" << std::setw(15) << "run 4"  << std::setw(15) << "run 5" << std::setw(15) << "average" << std::endl;

	for(memory_size_type i = 0; i < n; ++i) {
		std::cout << test_sizes[i] << std::setw(15) << std::flush;
		memory_size_type sum = 0;
		memory_size_type count = test_sizes[i] * 1024 * 1024 / sizeof(memory_size_type);

		for(memory_size_type j = 0; j < 5; ++j) {
			merge_sorter<memory_size_type, false> merge_sorter;

			merge_sorter.set_available_memory(1000 * 1024 * 1024);

			// Phase 1 - Push elements
			boost::mt19937 rng(42);
			boost::uniform_int<memory_size_type> dist(0, std::numeric_limits<memory_size_type>::max());
			boost::variate_generator<boost::mt19937&, boost::uniform_int<memory_size_type> > generator(rng, dist);

			Timer timer;
			{
				merge_sorter.begin();
				for(memory_size_type i = 0; i < count; ++i) {
					merge_sorter.push(generator());
				}
				merge_sorter.end();
			}
			memory_size_type phase1 = timer.elapsed();
			std::cout << phase1 << std::setw(15) << std::flush;
			sum += phase1;
		}

		std::cout << sum/5 << std::endl;
	}


	tpie_finish();
	return 0;
}
