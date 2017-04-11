#ifndef __MY_THREAD_POOL_H__
#define __MY_THREAD_POOL_H__

#include <pthread.h>
#include <stdlib.h>
#include <vector>
#include <stdio.h>
#define THREAD_NUM 12

struct linked_list_node_t
{
	linked_list_node_t *next;
	linked_list_node_t *prev;
};

template <typename T, linked_list_node_t T::*list_node>
class linked_list_t
{
	public:
		linked_list_t() { _head.next = _head.prev = &_head; }
		linked_list_t& operator =(const linked_list_t &) { _head.next = _head.prev = &_head; return *this; }
		bool is_empty() const { return _head.next == &_head; }
		void empty() { _head.next = _head.prev = &_head; }
		linked_list_node_t& head() { return _head; }
		T* entry(linked_list_node_t &node) const { return &node == &_head ? NULL : (T*)((char*)&node - (char*)_node_offset); }

		void add(T &node)
		{
			_head.next->prev = &(node.*list_node);
			(node.*list_node).next = _head.next;
			(node.*list_node).prev = &_head;
			_head.next = &(node.*list_node);
		}

		static void add_prev(T &node, T &cur)
		{
			(cur.*list_node).prev->next = &(node.*list_node);
			(node.*list_node).prev = (cur.*list_node).prev;
			(node.*list_node).next = &(cur.*list_node);
			(cur.*list_node).prev = &(node.*list_node);
		}

		static void add_next(T &node, T &cur)
		{
			(cur.*list_node).next->prev = &(node.*list_node);
			(node.*list_node).next = (cur.*list_node).next;
			(node.*list_node).prev = &(cur.*list_node);
			(cur.*list_node).next = &(node.*list_node);
		}

		static void del(T &node)
		{
			(node.*list_node).next->prev = (node.*list_node).prev;
			(node.*list_node).prev->next = (node.*list_node).next;
		}

		T* next(T &node) const
		{
			return (node.*list_node).next == &_head ? NULL : (T*)((char*)(node.*list_node).next - (char*)_node_offset);
		}

		T* prev(T &node) const
		{
			return (node.*list_node).prev == &_head ? NULL : (T*)((char*)(node.*list_node).prev - (char*)_node_offset);
		}

		T* next(linked_list_node_t &node) const
		{
			return node.next == &_head ? NULL : (T*)((char*)node.next - (char*)_node_offset);
		}

		T* prev(linked_list_node_t &node) const
		{
			return node.prev == &_head ? NULL : (T*)((char*)node.prev - (char*)_node_offset);
		}
#if 0

		void add(linked_list_node_t &node)
		{
			_head.next->prev = &node;
			node.next = _head.next;
			node.prev = &_head;
			_head.next = &node;
		}

		void del(linked_list_node_t &node)
		{
			node.next->prev = node.prev;
			node.prev->next = node.next;
		}
#endif

	protected:
		static linked_list_node_t const * const _node_offset;
		linked_list_node_t _head;
};

template <typename T, linked_list_node_t T::*list_node>
linked_list_node_t const * const linked_list_t<T, list_node>::_node_offset = &(((T *)0)->*list_node);










template <typename T, linked_list_node_t T::*list_node>
class wait_list_t: public linked_list_t<T, list_node>
{
	public:
		wait_list_t(): _alive(1),_num(0)
	{
		pthread_mutex_init(&_mutex, NULL);
		pthread_cond_init(&_cond, NULL);
	}

		~wait_list_t()
		{
			pthread_cond_destroy(&_cond);
			pthread_mutex_destroy(&_mutex);
		}

		int len()
		{
			return _num;
		}

		void put(T &node)
		{
			pthread_mutex_lock(&_mutex);
			if (_alive)
			{
				add(node);
				++_num;
			}
			pthread_cond_signal(&_cond);
			pthread_mutex_unlock(&_mutex);
		}

		T* get()
		{
			T *ret;
			pthread_mutex_lock(&_mutex);
			while (_alive && linked_list_t<T, list_node>::is_empty())
				pthread_cond_wait(&_cond, &_mutex);
			if (_alive)
			{
				ret = entry(*linked_list_t<T, list_node>::_head.prev);
				del(*ret);
				--_num;
			}
			else
			{
				ret = NULL;
			}
			pthread_mutex_unlock(&_mutex);
			return ret;
		}

		T* get(const struct timespec* abstime) 
		{ 
			T *ret; 
			pthread_mutex_lock(&_mutex); 
			while (_alive && linked_list_t<T, list_node>::is_empty()) 
			{ 
				if(pthread_cond_timedwait(&_cond,&_mutex,abstime) != 0)
				{ 
					pthread_mutex_unlock(&_mutex); 
					return NULL; 
				} 
			} 
			if (_alive) 
			{ 
				ret = entry(*linked_list_t<T, list_node>::_head.prev); 
				del(*ret); 
				--_num; 
			} 
			else 
			{ 
				ret = NULL; 
			} 
			pthread_mutex_unlock(&_mutex); 
			return ret; 
		} 

		T* get_from_head()
		{
			T *ret;
			pthread_mutex_lock(&_mutex);
			while (_alive && linked_list_t<T, list_node>::is_empty())
				pthread_cond_wait(&_cond, &_mutex);
			if (_alive)
			{
				ret = entry(*linked_list_t<T, list_node>::_head.next);
				del(*ret);
				--_num;
			}
			else
			{
				ret = NULL;
			}
			pthread_mutex_unlock(&_mutex);
			return ret;
		}

		void flush()
		{
			pthread_mutex_lock(&_mutex);
			_alive = 0;
			pthread_cond_broadcast(&_cond);
			pthread_mutex_unlock(&_mutex);
		}

	protected:
		pthread_mutex_t _mutex;
		pthread_cond_t _cond;
		int _alive;
		int _num;
};



class Worker
{
	public:
		Worker();
		virtual ~Worker(){}
	public:
		virtual int doWork(FILE * file)=0;
	public:
		linked_list_node_t task_list_node;
		unsigned char _state;	//0:worker尚未被处理 1:worker已经被处理完
		unsigned char _id;
};


class MyThreadPool {
	public:
		MyThreadPool();
		virtual ~MyThreadPool();
		virtual int open(size_t thread_num, size_t stack_size, FILE * (* binFiles)[THREAD_NUM]);
		virtual int activate();
		virtual int add_worker(Worker* worker);
		//线程回收，等待所有线程任务完成
		virtual int wait_worker_done(const std::vector<Worker*>& workers);
		//判断是否任务全部完成
		virtual bool is_worker_done(const std::vector<Worker*>& workers);
	private:
		static void* run_svc(void *arg);
		virtual int svc();
		virtual int join();
		virtual int stop();
	protected:
		pthread_t *m_thread;
		size_t m_thread_num;
		FILE * (* m_binfiles)[THREAD_NUM];
		pthread_barrier_t m_barrier;	
		wait_list_t<Worker, &Worker::task_list_node>  m_task_list;
	public:
		int routeID;
};




#endif //__MY_THREAD_POOL_H__

