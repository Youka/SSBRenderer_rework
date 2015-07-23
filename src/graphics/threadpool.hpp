/*
Project: SSBRenderer
File: threadpool.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <future>
#include <queue>
#include <stdexcept>

namespace stdex{
	// Pool of threads for recycling resources
	class threadpool{
		private:
			// Task executing threads
			std::vector<std::thread> workers;
			// Enqueued tasks for soon execution
			std::queue<std::function<void()>> tasks;
			// Interthread-communication helpers
			std::mutex mx;
			std::condition_variable cond;
			// Flag for threads finalization
			std::atomic_bool stop;
		public:
			// Constructor for complete pool
			threadpool(size_t threads_n = std::thread::hardware_concurrency()) throw(std::invalid_argument) : stop(false)/* std::atomic_bool refuses direct initialization in class because of missing copy constructor */{
				// No incomplete pool allowed
				if(!threads_n)
					throw std::invalid_argument("zero threads aren't an option");
				// Create workers/threads
				this->workers.reserve(threads_n);
				for(; threads_n; --threads_n)
					this->workers.emplace_back([](threadpool* tp){
						// Placeholder for upcoming task to execute
						std::function<void()> task;
						// Thread lifecircle
						while(true){
							// Scope for std::unique_lock
							{
								// Lock scope for threadsafe tasks access
								std::unique_lock<std::mutex> lock(tp->mx);
								// Wait when nothing to do but pool still active
								tp->cond.wait(lock, [tp](){return !tp->tasks.empty() || tp->stop;});
								// Finish when nothing to do and pool in destruction
								if(tp->tasks.empty() && tp->stop)
									return;
								// Prepare task for execution and remove out of queue
								task = std::move(tp->tasks.front());
								tp->tasks.pop();
							}
							// Task execution without lock
							task();
						}
					}, this);
			}
			// No copy or move of pool
			threadpool(const threadpool&) = delete;
			threadpool(threadpool&&) = delete;
			threadpool& operator=(const threadpool&) = delete;
			threadpool& operator=(threadpool&&) = delete;
			// Number of workers/running threads
			size_t size() const{
				return this->workers.size();
			}
			// Number of tasks in queue
			size_t pending_tasks(){
				std::unique_lock<std::mutex> lock(this->mx);
				return this->tasks.size();
			}
			// Add task of any format and get future object
			template<typename F, typename... A>
			std::future<typename std::result_of<F(A...)>::type> add_task(F func, A... args){
				// Pack function & arguments and get future object for threadsafe return value
				using packaged_task_t = std::packaged_task<typename std::result_of<F(A...)>::type () /* return type from original function and no arguments because of binding */>;
				std::shared_ptr<packaged_task_t> task(new packaged_task_t(std::bind(func, args...)));	// shared_ptr needed because of missing copy constructor of packaged_task (see following capture in lambda)
				auto fut = task->get_future();
				// Threadsafe tasks access
				{
					std::unique_lock<std::mutex> lock(this->mx);
					// Put packaged_task into function object to cover template arguments and add this to the queue
					this->tasks.emplace([task](){(*task)();});
				}
				// Wake one worker/thread up for the new task
				this->cond.notify_one();
				// Pass future object to user for waiting + return value
				return fut;
			}
			// Pool destruction + ending threads
			virtual ~threadpool(){
				this->stop = true;
				this->cond.notify_all();
				for(std::thread& worker : this->workers)
					worker.join();
			}
	};
}
