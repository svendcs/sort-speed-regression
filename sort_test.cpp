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

int main(int argc, char **argv) {
	typedef memory_size_type item_type;
	typedef std::less<item_type> pred_type;

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

	memory_size_type count = data / sizeof(item_type);
	merge_sorter<item_type, false, pred_type> merge_sorter;

	item_type e = 1;
	item_type prime = 26843545607;
	item_type multiplier = 38261;

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
			e = (e * multiplier) % prime;
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
	item_type hest = e;
	{
		report_timer timer("Third phase");
		#ifdef FASTER
		merge_sorter.pull_begin();
		#endif

		for(memory_size_type i = 0; i < count; ++i) {
			item_type j = merge_sorter.pull();
			hest *= j;
		}
	}

	if(hest == e) std::cout << "Det var dog underligt." << std::endl;

	// Clean-up
	tpie_finish();
	return 0;
}