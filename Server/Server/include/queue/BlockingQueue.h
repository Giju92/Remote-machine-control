#pragma once

#include <list>
#include <mutex>
#include <condition_variable>


template <class T>
class BlockingQueue
{
public:
	BlockingQueue(unsigned int dim);
	~BlockingQueue();
	bool get(T& res);
	void put(T val);
	int size();
	void close();

private:
	std::list<T> queue;
	unsigned int dim;
	bool isOpen;
	std::mutex mut;
	std::condition_variable cons_cond;
	std::condition_variable prod_cond;

};

template <typename T>
BlockingQueue<T>::BlockingQueue(unsigned int dim)
{
	this->dim = dim;
	isOpen = true;
}

template <typename T>
BlockingQueue<T>::~BlockingQueue()
{
}

template <typename T>
bool BlockingQueue<T>::get(T& res)
{
	/* Lock creation -> Nobody has to modify our variables */
	std::unique_lock<std::mutex> lk(mut);

	/* wait until the queue contains something or is closed */
	cons_cond.wait(lk, [this]() {return (queue.size() != 0 || !isOpen); });
	if (!isOpen && queue.size() == 0)
	{
		return false;
	}

	/* extract the element from the queue */
	res = queue.front();
	queue.pop_front();

	/* Wake up all threads */
	prod_cond.notify_all();

	return true;
}

template <typename T>
void BlockingQueue<T>::put(T val)
{
	std::unique_lock<std::mutex> lk(mut);

	prod_cond.wait(lk, [this]() {return (queue.size() != dim || !isOpen); });
	if (!isOpen) {
		//throw std::invalid_argument("The queue is closed...");
		return;
	}

	/* insert the element */
	queue.push_back(val);

	/* Wake up all consumers */
	cons_cond.notify_all();
}

template <typename T>
int BlockingQueue<T>::size()
{
	std::lock_guard<std::mutex> lk(mut);
	return (int)queue.size();
}

template <typename T>
void BlockingQueue<T>::close()
{
	std::lock_guard<std::mutex> lk(mut);
	isOpen = false;

	/* Notify all the producers */
	prod_cond.notify_all();

	/* Notify all the consumers */
	cons_cond.notify_all();
}
