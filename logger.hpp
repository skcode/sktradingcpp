#ifndef LOGGER_HPP_INCLUDED
#define LOGGER_HPP_INCLUDED
//**
//COMPILE with -rdynamic linker option
//**


#include <cxxabi.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <exception>
#include <stdexcept>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include <fstream>
#include <ostream>
#include <string>
#include <sstream>
#include <sys/time.h>

/// Comment this line if you don't need multithread support
#define LOGGER_MULTITHREAD
#ifdef LOGGER_MULTITHREAD
#include <pthread.h>
#endif
//using namespace std;

/**
 * \brief Macro to print log messages.
 */

#define LOG_DEBUG(msg) { \
	std::ostringstream __debug_stream__; \
	__debug_stream__ << msg; \
	LoggerNS::Logger::getInstance().print(LoggerNS::DBG_DEBUG, __FILE__, __LINE__, \
			__FUNCTION__,__debug_stream__.str()); \
	}
#define LOG_WARN(msg) { \
	std::ostringstream __debug_stream__; \
	__debug_stream__ << msg; \
	LoggerNS::Logger::getInstance().print(LoggerNS::DBG_WARN, __FILE__, __LINE__, \
			__FUNCTION__,__debug_stream__.str()); \
	}
#define LOG_ERROR(msg) { \
	std::ostringstream __debug_stream__; \
	__debug_stream__ << msg; \
	LoggerNS::Logger::getInstance().print(LoggerNS::DBG_ERROR, __FILE__, __LINE__, \
			__FUNCTION__,__debug_stream__.str()); \
	}
#define LOG_INFO(msg) { \
	std::ostringstream __debug_stream__; \
	__debug_stream__ << msg; \
	LoggerNS::Logger::getInstance().print(LoggerNS::DBG_INFO, __FILE__, __LINE__, \
			__FUNCTION__,__debug_stream__.str()); \
	}

#define THROW_EXCEPTION(ITEMS) {                                            \
  throw std::runtime_error( ( dynamic_cast<ostringstream &> (                             \
         ostringstream() . seekp( 0, ios_base::cur ) << ITEMS )   \
    ) . str() );\
}

/*
#define THROW_EXCEPTION(ITEMS) {                                            \
  LOG_ERROR(ITEMS);\
  throw std::runtime_error( ( dynamic_cast<ostringstream &> (                             \
         ostringstream() . seekp( 0, ios_base::cur ) << ITEMS )   \
    ) . str() );\
}
*/

namespace LoggerNS {


const int DBG_ERROR	= 0;
const int DBG_WARN	= 1;
const int DBG_INFO	= 2;
const int DBG_DEBUG	= 3;

/**
 * \brief Macro to configure the logger.
 * Example of configuration of the Logger:
 * 	DEBUG_CONF("outputfile", Logger::file_on|Logger::screen_on, DBG_DEBUG, DBG_ERROR);
 */
/*#define DEBUG_CONF(outputFile, \
		configuration, \
		fileVerbosityLevel, \
		screenVerbosityLevel) { \
			Logger::getInstance().configure(outputFile, \
						configuration, \
						fileVerbosityLevel, \
						screenVerbosityLevel); \
		}
*/




/**
 * \brief Simple logger to log messages on file and console.
 * This is the implementation of a simple logger in C++. It is implemented
 * as a Singleton, so it can be easily called through two DEBUG macros.
 * It is Pthread-safe.
 * It allows to log on both file and screen, and to specify a verbosity
 * threshold for both of them.
 */
class Logger
{
	/**
	 * \brief Type used for the configuration
	 */
	enum loggerConf_	{L_nofile_	= 	1 << 0,
				L_file_		=	1 << 1,
				L_noscreen_	=	1 << 2,
				L_screen_	=	1 << 3};

#ifdef LOGGER_MULTITHREAD
	/**
	 * \brief Lock for mutual exclusion between different threads
	 */
	static pthread_mutex_t lock_;
#endif

	bool configured_;

	/**
	 * \brief Pointer to the unique Logger (i.e., Singleton)
	 */
	static Logger* m_;

	/**
	 * \brief Initial part of the name of the file used for Logging.
	 * Date and time are automatically appended.
	 */
	std::string logFile_;

	/**
	 * \brief Current configuration of the logger.
	 * Variable to know if logging on file and on screen are enabled.
	 * Note that if the log on file is enabled, it means that the
	 * logger has been already configured, therefore the stream is
	 * already open.
	 */
	loggerConf_ configuration_;

	/**
	 * \brief Stream used when logging on a file
	 */
	std::ofstream out_;

	/**
	 * \brief Initial time (used to print relative times)
	 */
	struct timeval initialTime_;

	/**
	 * \brief Verbosity threshold for files
	 */
	unsigned int fileVerbosityLevel_;

	/**
	 * \brief Verbosity threshold for screen
	 */
	unsigned int screenVerbosityLevel_;

	Logger();
	~Logger();

	/**
	 * \brief Method to lock in case of multithreading
	 */
	inline static void lock();

	/**
	 * \brief Method to unlock in case of multithreading
	 */
	inline static void unlock();

public:

	typedef loggerConf_ loggerConf;
	static const loggerConf file_on= 	L_nofile_;
	static const loggerConf file_off= 	L_file_;
	static const loggerConf screen_on= 	L_noscreen_;
	static const loggerConf screen_off= L_screen_;

	static Logger& getInstance();

	void print(const unsigned int		verbosityLevel,
			const std::string&	sourceFile,
			const int 		codeLine,
			const std::string& func,
			const std::string& 	message);

	bool  configure (const std::string&	outputFile,
			const loggerConf	configuration,
			const int		fileVerbosityLevel,
			const int		screenVerbosityLevel);
};

inline Logger::loggerConf operator|
	(Logger::loggerConf __a, Logger::loggerConf __b)
{
	return Logger::loggerConf(static_cast<int>(__a) |
		static_cast<int>(__b));
}

inline Logger::loggerConf operator&
	(Logger::loggerConf __a, Logger::loggerConf __b)
{
	return Logger::loggerConf(static_cast<int>(__a) &
		static_cast<int>(__b)); }


static inline void printStacktrace( unsigned int max_frames = 63)
{
    std::stringstream s1;
    s1.clear();
    s1<< "stack trace:\n";

    //fprintf(out, "stack trace:\n");


    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {


        s1<< "  <empty, possibly corrupt>";
        LOG_ERROR(s1.str());
	return;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++)
    {
	char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

	// find parentheses and +address offset surrounding the mangled name:
	// ./module(function+0x15c) [0x8048a6d]
	for (char *p = symbollist[i]; *p; ++p)
	{
	    if (*p == '(')
		begin_name = p;
	    else if (*p == '+')
		begin_offset = p;
	    else if (*p == ')' && begin_offset) {
		end_offset = p;
		break;
	    }
	}

	if (begin_name && begin_offset && end_offset
	    && begin_name < begin_offset)
	{
	    *begin_name++ = '\0';
	    *begin_offset++ = '\0';
	    *end_offset = '\0';

	    // mangled name is now in [begin_name, begin_offset) and caller
	    // offset in [begin_offset, end_offset). now apply
	    // __cxa_demangle():

	    int status;
	    char* ret = abi::__cxa_demangle(begin_name,
					    funcname, &funcnamesize, &status);
	    if (status == 0) {
		funcname = ret; // use possibly realloc()-ed string
		s1<< symbollist[i]<<" "<<funcname<<" "<<begin_offset<<"\n";
		//fprintf(out, "  %s : %s+%s\n",
		//	symbollist[i], funcname, begin_offset);
	    }
	    else {
		// demangling failed. Output function name as a C function with
		// no arguments.

		s1<< symbollist[i]<<" "<<begin_name<<" "<<begin_offset<<"\n";
		//fprintf(out, "  %s : %s()+%s\n",
			//symbollist[i], begin_name, begin_offset);
	    }
	}
	else
	{
	    // couldn't parse the line? print the whole line.
	    s1<< symbollist[i]<<"\n";
	    //fprintf(out, "  %s\n", symbollist[i]);
	}
    }

    free(funcname);
    free(symbollist);
    LOG_ERROR(s1.str());
    //std::cerr<< s1.str();
}

static inline void signalHandler(int sig)
{
    std::stringstream s1;
    s1<<"Error: signal "<< sig;
    LOG_ERROR(s1.str());
    //std::fprintf(stderr, "Error: signal %d\n", sig);
    printStacktrace();
    std::abort();
}

static inline void terminateHandler()
{
    std::exception_ptr exptr = std::current_exception();
    std::stringstream s1;
    if (exptr != 0)
    {
        // the only useful feature of std::exception_ptr is that it can be rethrown...
        try
        {
            std::rethrow_exception(exptr);
        }
        catch (std::exception &ex)
        {
            s1<<"Terminated due to exception: "<<ex.what();
            //std::fprintf(stderr, "Terminated due to exception: %s\n", ex.what());
        }
        catch (...)
        {
            s1<<"Terminated due to unknown exception";
            //std::fprintf(stderr, "Terminated due to unknown exception\n");
        }
    }
    else
    {
        s1<<"Terminated due to unknown reason";
        //std::fprintf(stderr, "Terminated due to unknown reason :(\n");
    }
    LOG_ERROR(s1.str());
    printStacktrace();
    std::abort();
}






}

//global initialization
namespace {
    static const bool SET_TERMINATE_HANDLER =std::set_terminate(LoggerNS::terminateHandler);
    static const bool SET_SIGNAL_HANDLER=signal(SIGSEGV, LoggerNS::signalHandler);
    static const bool SET_LOGGER=LoggerNS::Logger::getInstance().configure("/tmp/outputfile",LoggerNS::Logger::file_off|LoggerNS::Logger::screen_on, LoggerNS::DBG_DEBUG, LoggerNS::DBG_DEBUG);
    //static const bool SET_LOGGER=LoggerNS::Logger::getInstance().configure("/tmp/outputfile",LoggerNS::Logger::file_off|LoggerNS::Logger::screen_off, LoggerNS::DBG_ERROR, LoggerNS::DBG_ERROR);
}

#endif // LOGGER_HPP_INCLUDED
