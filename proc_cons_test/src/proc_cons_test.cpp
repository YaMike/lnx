//============================================================================
// Name        : proc_cons_test.cpp
// Author      : Michael Likholet
// Version     :
// Copyright   : Your copyright notice
// Description : g++
//============================================================================

#include <iostream>
#include <thread>
#include <condition_variable>
#include <sstream>
#include <list>
#include <mutex>
#include <algorithm>
#include <vector>
#include <sstream>

#include <cstdlib>
#include <cstdint>
#include <ctime>

#include <pthread.h>

#define MORE_DETAILS
#undef MORE_DETAILS

struct Object {
	int dummy;
};

struct Message {
	Message(std::string d, int w): desc(d), what(w), arg1(0), arg2(0), ptr(NULL) {}
	std::string desc;
	int what;
	int arg1;
	int arg2;
	void *ptr;	// C++
	Object obj;	// Java
	friend std::ostream & operator<<(std::ostream &os, const Message& m);
};

std::ostream & operator<<(std::ostream &os, const Message& m) {
	std::stringstream strs;
	strs << "d: " << m.desc << ", what: " << m.what << ";";
	return os << strs.str();
}

/* MQueue file (mixed header/source */
class MQueue {
public:
	MQueue(const std::uint32_t size): _sz(size) { }
	~MQueue() {}

	/*
	 * Pushing message to queue: runtime is constant (O(1))
	 */
	bool
	push_msg(Message *m) {
		std::unique_lock<std::mutex> lock(_mtx);
		if (_ml.size() < _sz) {
			_ml.push_front(m);
		} else {
			while (_ml.size() >= _sz) {
				if (!_cv_popped.wait_for(lock, std::chrono::seconds(1) ,[this](){ return _ml.size() < _sz; })) {
					return false;
				}
			}
			_ml.push_front(m);
		}

		_msg_in.push_back(*m);
		_cv_pushed.notify_all();
		return true;
	}

	/*
	 * Retrieving the oldest message: runtime is constant (O(1))
	 */
	Message*
	pop_msg() {
		Message *oldest_msg = NULL;
		std::unique_lock<std::mutex> lock(_mtx);

		while (_ml.empty()) {
			if (!_cv_pushed.wait_for(lock, std::chrono::seconds(1), [this](){ return !_ml.empty(); })) {
				return NULL;
			}
		}
		oldest_msg = _ml.back();
		_msg_out.push_back(*oldest_msg);
		_ml.pop_back();
		_cv_popped.notify_all();
		return oldest_msg;
	}

	/*
	 * Obtaining the first (oldest message): runtime - constant time (O(1))
	 */
	Message*
	get_first_msg() { /* obtains the oldest message in the queue */
		std::unique_lock<std::mutex> lock(_mtx);

		if (_ml.empty()) {
			return NULL;
		} else {
			return _ml.back();
		}
	}

	/*
	 * Searching - O(N), deletion - constant (O(1)), method runtime - O(N)
	 */
	void
	delete_by_what(int w) {
		std::unique_lock<std::mutex> lock(_mtx);

		for (std::list<Message*>::iterator m_it = _ml.begin(); m_it != _ml.end();) {
			if ((*m_it)->what == w) {
				m_it = _ml.erase(m_it);
			} else {
				++m_it;
			}
		}
	}

	/*
	 * Naive test for correctness of the output
	 */
	void
	compare_results() {
		std::cout << "Comparing results" << std::endl;
		auto msg_cmp = [this](Message m1, Message m2) { return m1.what > m2.what; };

		std::sort(_msg_in.begin(), _msg_in.end(), msg_cmp);
		std::sort(_msg_out.begin(), _msg_out.end(), msg_cmp);

		if (_msg_in.size() != _msg_out.size()) {
			std::stringstream strs;
			strs << "Number of in and out messsages are different: in=" << _msg_in.size() << ", out=" << _msg_out.size();
			std::cerr << strs.str() << std::endl;
		}

		std::vector<Message>::iterator it_in  = _msg_in.begin();
		std::vector<Message>::iterator it_out = _msg_out.begin();
		for (; it_in != _msg_in.end() && it_out != _msg_out.end(); ++it_in, ++it_out) {
#ifdef MORE_DETAILS
			std::cout << "M_in: " << *it_in << "M_out: " << *it_out << "\n";
#endif

			if ((*it_in).what != (*it_out).what) {
				std::cerr << "M_in: " << *it_in << " differs from M_out: " << *it_out << "\n";
				return;
			}
		}

		std::cout << "Input and output messages are the same" << std::endl;
	}

private:
	const std::uint32_t _sz;

	std::mutex _mtx;
	std::condition_variable _cv_popped;
	std::condition_variable _cv_pushed;

	std::list<Message*> _ml; /* Choose list as it doesn't have penalty for insertion/deletion */

	std::vector<Message> _msg_in;
	std::vector<Message> _msg_out;
};

/* main test file */
#include <thread>
#include <chrono>
#include <random>
#include <sstream>

#include <cstdlib>

void consumer_func(std::string name, MQueue &mq, int msg_cnt)
{
	std::stringstream strs;
	strs << "t " << name << " started: consumption plan: " << msg_cnt << " msgs\n";
	std::cout << strs.str();

	while (msg_cnt-- > 0) {
		Message *m = mq.pop_msg();
		if (m == NULL) {
			std::cerr << "t " << name << ": timeout" << "\n";
			break;
		}
#ifdef MORE_DETAILS
		std::cout << "t " << name << ": got " << *m << "\n";
#endif
		delete m;
	}

	std::cout << "t " << name << " finished" << "\n";
}

void producer_func(std::string name, MQueue &mq, int msg_cnt)
{
	std::random_device rd;
	std::minstd_rand0 gen(rd());
	std::uniform_int_distribution<> what(1, msg_cnt);
	int msg_num = 0;

	std::stringstream strs;
	strs << "t " << name << " started: production plan: " << msg_cnt << " msgs\n";
	std::cout << strs.str();

	while (msg_cnt-- > 0) {
		Message *m = new Message(name, msg_num++);
		std::stringstream strs_msg;
		strs_msg << *m;
		if (!mq.push_msg(m)) {
			std::cerr << "t " << name << ": timeout" << "\n";
			break;
		}
#ifdef MORE_DETAILS
		std::cout << "t " << name << ": put " << strs_msg.str() << "\n";
#endif
	}

	std::cout << "t " << name << " finished" << "\n";
}

int main()
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> prod_cons_dis(5,6);
	std::uniform_int_distribution<int> msg_cnt_dis(100,300);

	const int cons_cnt = prod_cons_dis(generator);
	const int prod_cnt = prod_cons_dis(generator);

	//const int cons_cnt = 10;
	//const int prod_cnt = 2;
	const std::uint32_t mq_size = 2;

	std::vector<std::thread> threads;
	MQueue mq(mq_size);

	for (int i = 0; i < cons_cnt; i++) {
		std::stringstream strs;
		strs << "c_" << i;
		threads.push_back(std::thread(consumer_func, strs.str(), std::ref(mq), msg_cnt_dis(generator)));
		//threads.push_back(std::thread(consumer_func, strs.str(), std::ref(mq), 10));
	}

	for (int i = 0; i < prod_cnt; i++) {
		std::stringstream strs;
		strs << "p_" << i;
		threads.push_back(std::thread(producer_func, strs.str(), std::ref(mq), msg_cnt_dis(generator)));
		//threads.push_back(std::thread(producer_func, strs.str(), std::ref(mq), 50));
	}

	for (std::vector<std::thread>::iterator it = threads.begin(); it != threads.end(); it++) {
		(*it).join();
	}

	mq.compare_results();

	return 0;
}


