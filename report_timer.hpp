#ifndef TEST_REPORT_TIMER_HPP
#define TEST_REPORT_TIMER_HPP

#include <iostream>
#include "boost/date_time/posix_time/posix_time.hpp"

struct report_timer {
	report_timer(const std::string & name) : name(name) {
		start = boost::posix_time::microsec_clock::local_time();
	}

	size_t elapsed() {
		boost::posix_time::ptime current = boost::posix_time::microsec_clock::local_time();
		return (current-start).total_milliseconds();
	}

	~report_timer() {
		std::cout << name << ": " << this->elapsed() / 1000 << " seconds elapsed." << std::endl;
	}
private:
	std::string name;
	boost::posix_time::ptime start;
};

#endif