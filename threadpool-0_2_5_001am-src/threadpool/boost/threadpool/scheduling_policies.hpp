/*! \file
* \brief Task scheduling policies.
*
* This file contains some fundamental scheduling policies for the pool class. 
* A scheduling policy is realized by a task container which controls the access to
* the tasks. 	Fundamentally the container determines the order the tasks are processed
* by the thread pool. 
* The task containers need not to be thread-safe because they are used by the pool 
* in thread-safe way. 
*
* Copyright (c) 2005-2007 Philipp Henkel
*
* Use, modification, and distribution are  subject to the
* Boost Software License, Version 1.0. (See accompanying  file
* LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*
* http://threadpool.sourceforge.net
*
*/


#ifndef THREADPOOL_SCHEDULING_POLICIES_HPP_INCLUDED
#define THREADPOOL_SCHEDULING_POLICIES_HPP_INCLUDED


#include <queue>
#include <deque>
#include <map>

#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "task_adaptors.hpp"

namespace boost { namespace threadpool
{

  /*! \brief SchedulingPolicy which implements FIFO ordering. 
  *
  * This container implements a FIFO scheduling policy.
  * The first task to be added to the scheduler will be the first to be removed.
  * The processing proceeds sequentially in the same order. 
  * FIFO stands for "first in, first out".
  *
  * \param Task A function object which implements the operator()(void).
  *
  */ 
  template <typename Task = task_func>  
  class fifo_scheduler
  {
  public:
    typedef Task task_type; //!< Indicates the scheduler's task type.
    typedef void*  queue_id_type; //<! For active objects. Used only by the active_scheduler
  

  protected:
    std::deque<task_type> m_container;  //!< Internal task container.	


  public:
    /*! Adds a new task to the scheduler.
    * \param task The task object.
    * \return true, if the task could be scheduled and false otherwise. 
    */
    bool push(task_type const & task)
    {
      m_container.push_back(task);
      return true;
    }

    /*! Gets the task which should be executed next & removes it.
    *  \return The task object to be executed.
    */
    task_type pop_top()
    {
      task_type task = m_container.top();
      m_container.pop();
      return task;
    }

    /*! Gets the current number of tasks in the scheduler.
    *  \return The number of tasks.
    *  \remarks Prefer empty() to size() == 0 to check if the scheduler is empty.
    */
    size_t size() const
    {
      return m_container.size();
    }

    /*! Checks if the scheduler is empty.
    *  \return true if the scheduler contains no tasks, false otherwise.
    *  \remarks Is more efficient than size() == 0. 
    */
    bool empty() const
    {
      return m_container.empty();
    }

    /*! Removes all tasks from the scheduler.
    */  
    void clear()
    {   
      m_container.clear();
    } 
  };



  /*! \brief SchedulingPolicy which implements LIFO ordering. 
  *
  * This container implements a LIFO scheduling policy.
  * The last task to be added to the scheduler will be the first to be removed.
  * LIFO stands for "last in, first out".
  *
  * \param Task A function object which implements the operator()(void).
  *
  */ 
  template <typename Task = task_func>  
  class lifo_scheduler
  {
  public:
    typedef Task task_type;  //!< Indicates the scheduler's task type.
    typedef void*  queue_id_type; 

  protected:
    std::deque<task_type> m_container;  //!< Internal task container.	

  public:
    /*! Adds a new task to the scheduler.
    * \param task The task object.
    * \return true, if the task could be scheduled and false otherwise. 
    */
    bool push(task_type const & task)
    {
      m_container.push_front(task);
      return true;
    }

    /*! Gets the task which should be executed next & removes it.
    *  \return The task object to be executed.
    */
    task_type pop_top()
    {
      task_type task = m_container.top();
      m_container.pop();
      return task;
    }

    /*! Gets the current number of tasks in the scheduler.
    *  \return The number of tasks.
    *  \remarks Prefer empty() to size() == 0 to check if the scheduler is empty.
    */
    size_t size() const
    {
      return m_container.size();
    }

    /*! Checks if the scheduler is empty.
    *  \return true if the scheduler contains no tasks, false otherwise.
    *  \remarks Is more efficient than size() == 0. 
    */
    bool empty() const
    {
      return m_container.empty();
    }

    /*! Removes all tasks from the scheduler.
    */  
    void clear()
    {    
      m_container.clear();
    } 

  };



  /*! \brief SchedulingPolicy which implements prioritized ordering. 
  *
  * This container implements a scheduling policy based on task priorities.
  * The task with highest priority will be the first to be removed.
  * It must be possible to compare two tasks using operator<. 
  *
  * \param Task A function object which implements the operator() and operator<. operator< must be a partial ordering.
  *
  * \see prio_thread_func
  *
  */ 
  template <typename Task = prio_task_func>  
  class prio_scheduler
  {
  public:
    typedef Task task_type; //!< Indicates the scheduler's task type.
    typedef void*  queue_id_type; 

  protected:
    std::priority_queue<task_type> m_container;  //!< Internal task container.


  public:
    /*! Adds a new task to the scheduler.
    * \param task The task object.
    * \return true, if the task could be scheduled and false otherwise. 
    */
    bool push(task_type const & task)
    {
      m_container.push(task);
      return true;
    }

    /*! Gets the task which should be executed next & removes it.
    *  \return The task object to be executed.
    */
    task_type pop_top()
    {
      task_type task = m_container.top();
      m_container.pop();
      return task;
    }
    
    /*! Gets the current number of tasks in the scheduler.
    *  \return The number of tasks.
    *  \remarks Prefer empty() to size() == 0 to check if the scheduler is empty.
    */
    size_t size() const
    {
      return m_container.size();
    }

    /*! Checks if the scheduler is empty.
    *  \return true if the scheduler contains no tasks, false otherwise.
    *  \remarks Is more efficient than size() == 0. 
    */
    bool empty() const
    {
      return m_container.empty();
    }

    /*! Removes all tasks from the scheduler.
    */  
    void clear()
    {    
      while(!m_container.empty())
      {
        m_container.pop();
      }
    } 
  };


  template <typename Task = task_func>  
  class active_scheduler
  {
  public:
    typedef Task   task_type; //!< Indicates the scheduler's task type.
    typedef void*  queue_id_type; 
    typedef long   key_type;

  protected:
    typedef std::deque<task_type>                             queue_type;
    typedef std::pair<queue_id_type, shared_ptr<queue_type> > queue_map_pair_type;
    typedef std::map<queue_id_type, shared_ptr<queue_type> >  queue_map_type;
    typedef std::deque<shared_ptr<queue_type> >               queue_list_type;
    
    queue_map_type   m_queues;
    queue_list_type  m_schedulable_queues;
    
    static boost::random::uniform_int_distribution<> m_rnd;
    static boost::random::mt19937                    m_rng;
    


  public:
    /*! Adds a new task to the scheduler.
    * \param task The task object.
    * \return true, if the task could be scheduled and false otherwise. 
    */
    bool push(queue_id_type queue_id, task_type const & task)
    {
      typename queue_map_type::iterator qit = m_queues.find(queue_id);
      if (qit == m_queues.end()) {
        shared_ptr<queue_type> q(new queue_type());
        m_queues.insert(queue_map_pair_type(queue_id, q));
	qit = m_queues.find(queue_id);
	
	m_schedulable_queues.push_back(q);
      }
      
      qit->second->push_back(task);
      return true;
    }

    /*! Gets the task which should be executed next & removes it.
    *  \return The task object to be executed.
    */
    task_type pop_top()
    {
      shared_ptr<queue_type> qit = m_schedulable_queues.front();
      m_schedulable_queues.pop_front();
      
      task_type task = qit->front();
      qit->pop_front();
      
      return task;
    }
    
    bool requeue(queue_id_type queue_id)
    {
      typename queue_map_type::iterator qit = m_queues.find(queue_id);
      if (qit->second->empty()) {
        m_queues.erase(qit);
      }
      else {
	m_schedulable_queues.push_back(qit->second);
      }
      
      return true;
    }

    /*! Gets the current number of tasks in the scheduler.
    *  \return The number of tasks.
    *  \remarks Prefer empty() to size() == 0 to check if the scheduler is empty.
    */
    size_t size() const
    {
      return m_schedulable_queues.size();
    }

    /*! Checks if the scheduler is empty.
    *  \return true if the scheduler contains no tasks, false otherwise.
    *  \remarks Is more efficient than size() == 0. 
    */
    bool empty() const
    {
      return m_schedulable_queues.empty();
    }

    /*! Removes all tasks from the scheduler.
    */  
    void clear()
    {   
      m_schedulable_queues.clear();
      m_queues.clear();
    } 
    
    static key_type key()
    {
      return m_rnd(m_rng);
    }
    
    static key_type key(void * ptr)
    {
      return static_cast<key_type>(ptr);
    }
  };



} } // namespace boost::threadpool


#endif // THREADPOOL_SCHEDULING_POLICIES_HPP_INCLUDED

