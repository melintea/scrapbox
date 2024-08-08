/*! \file
* \brief Active object and helpers.
*
* http://niki.code-karma.com/2011/02/zeromq-the-active-object-pattern/
*/


#ifndef ACTIVE_OBJECT_HPP_INCLUDED
#define ACTIVE_OBJECT_HPP_INCLUDED

#include <boost/ref.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/smart_ptr.hpp>

#include <boost/threadpool.hpp>




namespace active { 



// multiple writer, multiple consumer
// based on Anthony Williams implementation (with added support for bounded size)
// Anthony Williams is the current maintainer of boost::thread
// http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
template<typename T>
class concurrent_queue {
public:
 
	concurrent_queue():max_elements(0) {}
	explicit concurrent_queue(size_t max):max_elements(max) {}
 
	// pushes an entry onto the queue.
	// if the queue is at maximum, the current thread waits
	// this helps us avoid producers outpacing the consumer(s) and causing OOM
	void push(const T& v) {
		boost::mutex::scoped_lock l(m_mutex);
		while(max_elements!=0 && m_queue.size() >= max_elements )
		{
			m_cond.wait(l);
		}
		m_queue.push(v);
		l.unlock();
		m_cond.notify_one();
	}
 
	// pops an element off the queue and returns it
	// if there are no elements in the queue the current thread waits
	void pop(T& v) {
		boost::mutex::scoped_lock l(m_mutex);
		while(m_queue.empty())
		{
			m_cond.wait(l);
		}
		// we cant return by value and maintain strong exception safety because copy ctors can throw
		// if it throws on the return we would have already done the pop. 
		// see http://www.gotw.ca/gotw/008.htm
		v = m_queue.front();
		m_queue.pop();
		m_cond.notify_one();
	}
 
	// no guarantee that this is accurate as soon as its returned.
	// but may be useful for diagnostics
	bool empty() const {
		boost::mutex::scoped_lock l(m_mutex);
		return m_queue.empty();
	}
 
	// no guarantee that this is accurate as soon as its returned.
	// but may be useful for diagnostics
	size_t size() const{
		boost::mutex::scoped_lock l(m_mutex);
		return m_queue.size();
	}
 
	size_t max_size () const {
		boost::mutex::scoped_lock l(m_mutex);
		return max_elements;
	}
 
private:
	mutable boost::mutex m_mutex;
	std::queue<T> m_queue;
	size_t max_elements;
	boost::condition_variable m_cond;
};



typedef boost::threadpool::thread_pool<
				 boost::threadpool::task_func, 
                                 boost::threadpool::active_scheduler,
                                 boost::threadpool::static_size,
                                 boost::threadpool::resize_controller,
                                 boost::threadpool::wait_for_all_tasks>  threadpool;


// helper for the Active Object pattern
// see Sutters article at http://www.drdobbs.com/high-performance-computing/225700095
class active_object_helper {
public:

public:
	active_object_helper(threadpool& tp) 
	    : m_exit(false) 
	    , m_threadpool(tp)  {
		m_thread.reset( new boost::thread( boost::bind(&active_object_helper::run, this) ) );
	}
 
	~active_object_helper() {
		schedule( boost::bind(&active_object_helper::exit, this) );
		// wait for queue to drain and thread to exit
		m_thread->join();
	}
 
	void schedule(const boost::function0<void>& f) {m_queue.push(f);}
private:
 
	// gets run on the launched thread
	void run() {
		boost::function0<void> f;
		while (true){
			m_queue.pop(f);
			f();
			f.clear();
			if (m_exit)
				return;
		}
	}
 
	// a message we use to exit the thread
	void exit() { m_exit = true; }
 
	concurrent_queue< boost::function0<void> > m_queue;
	boost::scoped_ptr<boost::thread> m_thread;
	bool m_exit;
	threadpool& m_threadpool;
};



/*
class tick_publisher {
   virtual void tick(const MarketData& md) = 0;
   virtual ~tick_publisher() {}
};
 
class zmq_tick_publisher: public tick_publisher {
public:
	zmq_tick_publisher(zmq::context_t& ctx, const std::string& bind_address) {
		m_active_object.schedule( boost::bind(&zmq_tick_publisher::init, this, boost::ref(ctx), bind_address) );
	}
	virtual ~zmq_tick_publisher(){
		m_active_object.schedule( boost::bind(&zmq_tick_publisher::deinit, this) );
	}
	virtual void tick(const MarketData& md) {
		m_active_object.schedule( boost::bind(&zmq_tick_publisher::tick_, this, md) );
	}
private:
	void init(zmq::context_t& ctx, const std::string& bind_address) {
		// setup socket
		m_socket = new zmq::socket_t(ctx, ZMQ_PUB);
		m_socket->bind(bind_address.c_str());
	}
	void deinit(){
		// teardown socket
		delete m_socket;
	}
	void tick_(const MarketData& md){
		// encode and broadcast on socket
		zmq::message_t msg;
		encode(md, msg);
		bool success = m_socket->send(msg);
		assert(success);
	}
	active_object_helper m_active_object;
	zmq::socket_t* m_socket;
};


main() {
 // ...
 std::string bind_address = settings.get().getString("BindAddress"); // eg "tcp://*:5000"
 
 boost::shared_ptr<tick_publisher> tick_pub(new zmq_tick_publisher(zmq_ctx, bind_address) );
 MQFeederApplication(settings, tick_pub);
 // ...
}
 
MQFeederApplication::MQFeederApplication(const FIX::SessionSettings& s, boost::shared_ptr<tick_publisher> publisher) 
	:m_settings(s), m_publisher(publisher)
{
}
 
void MQFeederApplication::onMessage( const FIX44::MarketDataSnapshotFullRefresh& m, const FIX::SessionID& sessionID)
{
	FIX::Symbol s = FIELD_GET_REF(m, Symbol);
	MarketData md(s);
	// fill in market data
	// ==snip==
	// publish
	m_publisher->tick(md);
}
*/


} // namespace active

#endif // ACTIVE_OBJECT_HPP_INCLUDED

