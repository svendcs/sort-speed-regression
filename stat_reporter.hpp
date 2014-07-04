#ifndef TEST_STAT_REPORTER_HPP
#define TEST_STAT_REPORTER_HPP

#ifndef WIN32

#include <sys/time.h>
#include <sys/resource.h>
#include "boost/date_time/posix_time/posix_time.hpp"

///////////////////////////////////////////////////////////////////////////////
/// Report CPU usage
///////////////////////////////////////////////////////////////////////////////
uint64_t get_cputime() {
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	return (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec )*1000 +
		(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec )/1000;
}

#else //WIN32
#include <Windows.h>

size_t get_cputime() {
	HANDLE self;
	FILETIME ftime, fsys, fuser;

	self = GetCurrentProcess();
	GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
	ULARGE_INTEGER sys;
	sys.LowPart = fsys.dwLowDateTime;
	sys.HighPart = fsys.dwHighDateTime;

	ULARGE_INTEGER user;
	user.LowPart = fuser.dwLowDateTime;
	user.HighPart = fuser.dwHighDateTime;
	return (sys.QuadPart + user.QuadPart)/10000;
}

#endif //! WIN32

///////////////////////////////////////////////////////////////////////////////
/// Report memory usage
///////////////////////////////////////////////////////////////////////////////
size_t memoryUsage() {
	return tpie::get_memory_manager().used();
}

///////////////////////////////////////////////////////////////////////////////
/// Report external memory usage
///////////////////////////////////////////////////////////////////////////////
uint64_t ioBytes() {
	return tpie::get_bytes_read() + tpie::get_bytes_written();
}
uint64_t tempUsage() {
	return tpie::get_temp_file_usage();
}


class StatReporter
{
public:
	const size_t captureInterval = 2;

	StatReporter(const std::string & fileName) : m_file(fileName) {
		m_file << "# time cpu_usage memory_usage io_bytes" << std::endl;
	}

	~StatReporter() {
		m_file.close();
	}

	void run() {
		m_cpuTime = get_cputime();
		m_time = m_start = boost::posix_time::microsec_clock::local_time();
		m_thread = new boost::thread(boost::bind(&StatReporter::thread, this));
	}

	void stop() {
		if(!m_thread) return;
		m_thread->interrupt();
		m_thread->join();
		delete m_thread;
		m_thread = 0;
	}
private:
	void thread() {
		boost::xtime xt;
		#if BOOST_VERSION >= 105000
			xtime_get(&xt, boost::TIME_UTC_);
		#else
			xtime_get(&xt, boost::TIME_UTC);
		#endif

		try {
			while (true) {
				capture();
				xt.sec += captureInterval;
				boost::this_thread::sleep(xt);
			}
		} catch(boost::thread_interrupted) {}

	}

	void capture() {
		// calculate cpu usage
		float cpu_time_diff = get_cputime() - m_cpuTime;
		float real_time_diff = (boost::posix_time::microsec_clock::local_time() - m_time).total_milliseconds();
		float cpu_usage = cpu_time_diff / real_time_diff * 100.f;

		// get time since start
		size_t real_time = (boost::posix_time::microsec_clock::local_time() - m_start).total_milliseconds();

		// get memory usage
		float memory_usage = memoryUsage();

		// get the number of bytes written and read
		float io_bytes = ioBytes();

		// output
		m_file << real_time << " " << cpu_usage << " " << memory_usage << " " << io_bytes << std::endl;

		// update variables
		m_cpuTime = get_cputime();
		m_time = boost::posix_time::microsec_clock::local_time();
	}

	size_t m_cpuTime;
	boost::posix_time::ptime m_time;
	boost::posix_time::ptime m_start;
	boost::thread * m_thread;
	std::ofstream m_file;
};

#endif