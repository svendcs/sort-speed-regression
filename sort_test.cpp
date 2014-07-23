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
#include "report_timer.hpp"

using namespace tpie;

void usage() {
	std::cout << "Parameters: [report_file] [data/MiB = 10] [memory/MiB = 50]" << std::endl;
}

struct big_item {
	memory_size_type a, b, c, d, e, f, g, h;

	big_item() {}

	big_item(memory_size_type i) : a(i), b(i), c(i), d(i), e(i), f(i), g(i), h(i) {}

	bool operator<(const big_item & other) const {
		return a < other.a;
	}
};

int main(int argc, char **argv) {
	// default parameters
	std::string report_file;
	size_t data = 10;
	size_t memory = 50;

	// parse parameters
	if(argc <= 1) {
		usage();
		return EXIT_FAILURE;
	}

	if(argc > 1) {
		report_file = std::string(argv[1]);
	}

	if(argc > 2) {
		std::stringstream(argv[2]) >> data;
		if(!data) {
			usage();
			return EXIT_FAILURE;
		}
	}
	if(argc > 3) {
		std::stringstream(argv[3]) >> memory;
		if(!memory) {
			usage();
			return EXIT_FAILURE;
		}
	}

	// normalize parameters
	data *= 1024 * 1024;
	memory *= 1024 * 1024;

	// Output header
	std::cout << "Report file: \"" << report_file << "\"" << std::endl;
	std::cout << "Data: " << data/1024/1024 << "MiB" << std::endl;
	std::cout << "Memory: " << memory/1024/1024 << "MiB" << std::endl;

	// init
	tpie::tpie_init();

	memory_size_type count = data / sizeof(big_item);
	merge_sorter<big_item, false, std::less<big_item> > merge_sorter;

	big_item e(1);
	memory_size_type prime = 26843545607;
	memory_size_type multiplier = 38261;

	// Initialization phase: set parameters
	{
		report_timer timer("Initialization phase");
		merge_sorter.set_available_memory(memory);
	}

	// First phase: Push items
	StatReporter reporter(report_file);
	reporter.run();

	{
		report_timer timer("First phase");
		merge_sorter.begin();
		for(memory_size_type i = 0; i < count; ++i) {
			merge_sorter.push(e);
			e.a = (e.a * multiplier) % prime;
		}
		merge_sorter.end();
	}

	// Second phase: Perform potential merges
	dummy_progress_indicator pi;
	{
		report_timer timer("Second phase");
		merge_sorter.calc(pi);
	}

	// Phase 3 - Pull
	big_item hest = e;
	{
		report_timer timer("Third phase");
		#ifdef FASTER
		merge_sorter.pull_begin();
		#endif

		for(memory_size_type i = 0; i < count; ++i) {
			big_item j = merge_sorter.pull();
			hest.a *= j.a;
		}
	}

	if(!(hest < e) && !(e < hest)) std::cout << "Det var dog underligt." << std::endl;

	// Clean-up
	tpie_finish();
	return 0;
}