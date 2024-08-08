/*! \file
 * \brief Multi-queue test bed.
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * http://threadpool.sourceforge.net
 *
 */


#include <boost/threadpool.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/ref.hpp>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "active.hpp"


using namespace std;
using namespace boost::threadpool;

//
// Helpers

boost::mutex g_io_monitor;

void print(string text)
{
  cout << text;
}

template<class T>
string to_string(const T& value)
{
  ostringstream ost;
  ost << value;
  ost.flush();
  return ost.str();
}

unsigned long get_ms_diff(boost::xtime& start, boost::xtime& end)
{
  boost::xtime::xtime_sec_t start_ms = start.sec * 1000	+ start.nsec/1000000; 
  boost::xtime::xtime_sec_t end_ms = end.sec * 1000	+ end.nsec/1000000; 
  return static_cast<unsigned long>(end_ms - start_ms);
}


struct ball
{
    ball(const std::string& name) : m_name(name) {}
    std::string m_name;
};



class seven_tentacle_octopus {
public:
        static const int num_tentacles = 7;
	static const int num_max_juggles = 100;
	
public:
	seven_tentacle_octopus(const std::string& name, active::threadpool tp) 
	  : m_threadpool(tp)
	  , m_my_name(name) 
	  , m_num_juggles(0)
	  , m_running(false) {
	    {
                boost::mutex::scoped_lock lock(g_io_monitor);
	        std::cout << "seven_tentacle_octopus::" << m_my_name << " " << this << std::endl;
	    }
	  }
	
	void tentacle0(ball& aball, seven_tentacle_octopus& other) {
	    check();
	    
	    {
                boost::mutex::scoped_lock lock(g_io_monitor);
	        std::cout << "T" << boost::this_thread::get_id() << ": seven_tentacle_octopus::" << m_my_name << " " << __FUNCTION__ << "(" << aball.m_name << ") from " << other.m_my_name << std::endl;
	    }
	    
	    //sleep
	    
	    m_threadpool.schedule( &other, //boost::threadpool::queue_id_type(&other), 
	                           boost::bind<void>(&seven_tentacle_octopus::tentacle1, &other, boost::ref(aball), boost::ref(*this)) );
	
	    uncheck();
	}
	
	void tentacle1(ball& aball, seven_tentacle_octopus& other) {
	    check();
	    
	    {
                boost::mutex::scoped_lock lock(g_io_monitor);
	        std::cout << "T" << boost::this_thread::get_id() << ": seven_tentacle_octopus::" << m_my_name << " " << __FUNCTION__ << "(" << aball.m_name << ") from " << other.m_my_name << std::endl;
	    }
	    
	    //sleep
	    
	    m_threadpool.schedule( &other, 
	                           boost::bind<void>(&seven_tentacle_octopus::tentacle2, &other, boost::ref(aball), boost::ref(*this)) );
	
	    uncheck();
	}
	void tentacle2(ball& aball, seven_tentacle_octopus& other) {
	    check();
	    
	    {
                boost::mutex::scoped_lock lock(g_io_monitor);
	        std::cout << "T" << boost::this_thread::get_id() << ": seven_tentacle_octopus::" << m_my_name << " " << __FUNCTION__ << "(" << aball.m_name << ") from " << other.m_my_name << std::endl;
	    }
	    
	    //sleep
	    
	    m_threadpool.schedule( &other, 
	                           boost::bind<void>(&seven_tentacle_octopus::tentacle3, &other, boost::ref(aball), boost::ref(*this)) );
	
	    uncheck();
	}
	
	void tentacle3(ball& aball, seven_tentacle_octopus& other) {
	    check();
	    
	    {
                boost::mutex::scoped_lock lock(g_io_monitor);
	        std::cout << "T" << boost::this_thread::get_id() << ": seven_tentacle_octopus::" << m_my_name << " " << __FUNCTION__ << "(" << aball.m_name << ") from " << other.m_my_name << std::endl;
	    }
	    
	    //sleep
	    
	    m_threadpool.schedule( &other, 
	                           boost::bind<void>(&seven_tentacle_octopus::tentacle4, &other, boost::ref(aball), boost::ref(*this)) );
	
	    uncheck();
	}
	
	void tentacle4(ball& aball, seven_tentacle_octopus& other) {
	    check();
	    
	    {
                boost::mutex::scoped_lock lock(g_io_monitor);
	        std::cout << "T" << boost::this_thread::get_id() << ": seven_tentacle_octopus::" << m_my_name << " " << __FUNCTION__ << "(" << aball.m_name << ") from " << other.m_my_name << std::endl;
	    }
	    
	    //sleep
	    
	    m_threadpool.schedule( &other, 
	                           boost::bind<void>(&seven_tentacle_octopus::tentacle5, &other, boost::ref(aball), boost::ref(*this)) );
	
	    uncheck();
	}
	
	void tentacle5(ball& aball, seven_tentacle_octopus& other) {
	    check();
	    
	    {
                boost::mutex::scoped_lock lock(g_io_monitor);
	        std::cout << "T" << boost::this_thread::get_id() << ": seven_tentacle_octopus::" << m_my_name << " " << __FUNCTION__ << "(" << aball.m_name << ") from " << other.m_my_name << std::endl;
	    }
	    
	    //sleep
	    
	    m_threadpool.schedule( &other, 
	                           boost::bind<void>(&seven_tentacle_octopus::tentacle6, &other, boost::ref(aball), boost::ref(*this)) );
	
	    uncheck();
	}
	
	void tentacle6(ball& aball, seven_tentacle_octopus& other) {
	    check();
	    
	    {
                boost::mutex::scoped_lock lock(g_io_monitor);
	        std::cout << "T" << boost::this_thread::get_id() << ": seven_tentacle_octopus::" << m_my_name << " " << __FUNCTION__ << "(" << aball.m_name << ") from " << other.m_my_name << std::endl;
	    }
	    
	    //sleep
	    
	    if (++m_num_juggles < num_max_juggles) {
	        m_threadpool.schedule( &other, 
		                       boost::bind<void>(&seven_tentacle_octopus::tentacle0, &other, boost::ref(aball), boost::ref(*this)) );
	    }
	    //else drop the ball
	
	    uncheck();
	}
	
private:

	void check() {
	    boost::recursive_mutex::scoped_lock scoped_lock(m_monitor);
	    assert(false == m_running);
	    m_running = true;
	}
	
	void uncheck() {
	    boost::recursive_mutex::scoped_lock scoped_lock(m_monitor);
	    assert(true == m_running);
	    m_running = false;
	}
	
private:
	active::threadpool&   m_threadpool;
	
	std::string  m_my_name;
	int m_num_juggles;
        mutable boost::recursive_mutex  m_monitor;
	
	volatile bool m_running; 
};





//
// A demonstration of the thread_pool class
int main (int argc, char * const argv[]) 
{
  print("MAIN: construct thread pool\n");


  boost::xtime start;
  boost::xtime_get(&start, boost::TIME_UTC);



  boost::threadpool::thread_pool<boost::threadpool::task_func, 
                                 boost::threadpool::active_scheduler,
                                 boost::threadpool::static_size,
                                 boost::threadpool::resize_controller,
                                 boost::threadpool::wait_for_all_tasks>  tp;
  tp.size_controller().resize(2*seven_tentacle_octopus::num_tentacles);	
  
  seven_tentacle_octopus juggler1("juggler1", tp);
  seven_tentacle_octopus juggler2("juggler2", tp);
  ball ball01("ball01"), ball02("ball02"), ball03("ball03"), ball04("ball04"), ball05("ball05"), ball06("ball06"), ball07("ball07"), 
       ball08("ball08"), ball09("ball09"), ball10("ball10"), ball11("ball11"), ball12("ball12"), ball13("ball13"), ball14("ball14"); 
  
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball01), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball02), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball03), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball04), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball05), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball06), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball07), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball08), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball09), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball10), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball11), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball12), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball13), boost::ref(juggler2)) );
  tp.schedule( &juggler1, boost::bind<void>(&seven_tentacle_octopus::tentacle0, boost::ref(juggler1), boost::ref(ball14), boost::ref(juggler2)) );

  tp.wait();

  boost::xtime end;
  boost::xtime_get(&end, boost::TIME_UTC);				

  print("\nMAIN: duration " + to_string(get_ms_diff(start, end)) + " ms \n");

  return 0;
}
