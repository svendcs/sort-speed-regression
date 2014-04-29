#ifndef TEST_STAT_REPORTER_HPP
#define TEST_STAT_REPORTER_HPP

#ifndef WIN32

#include <sys/time.h>
#include <sys/resource.h>

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
		m_file << "# cpuUsage memoryUsage ioBytes" << std::endl;
	}

	~StatReporter() {
		m_file.close();
	}

	void run() {
		m_cpuTime = get_cputime();
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
		size_t cpu_time = get_cputime();
		size_t io = ioBytes();

		float cpu_usage = float(cpu_time - m_cpuTime)/float(captureInterval);
		float memory_usage = memoryUsage();
		float io_bytes = ioBytes();
		m_file << cpu_usage << " " << memory_usage << " " << io_bytes << std::endl;

		m_cpuTime = cpu_time;
	}

	size_t m_cpuTime;
	std::ofstream m_file;
	boost::thread * m_thread;
};

#endif