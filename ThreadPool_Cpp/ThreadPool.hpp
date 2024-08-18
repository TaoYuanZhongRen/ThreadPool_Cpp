#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>
#include <functional>
#include <queue>

//�̳߳�
class ThreadPool
{
public:
	//���캯��
	ThreadPool(int threadnums);
	//��������
	~ThreadPool();
	//�������
	template <typename F, typename ...Arg>
	auto enques(F&& f, Arg&&... arg) -> std::future<typename std::result_of<F(Arg...)>::type>;
private:
	void worker();//�̵߳�ִ������
	bool isstop; //��ʶ ��ǰ�̳߳��ǲ���ֹͣ trueֹͣ
	std::condition_variable cv;//��������;
	std::mutex mtx;//������
	std::vector<std::thread>workers;//�̼߳���(�̳߳�)
	std::queue<std::function<void()>>myque;//�������
};

ThreadPool::ThreadPool(int threadnums) :isstop(false)
{
	for (size_t i = 0; i < threadnums; i++)
	{
		workers.emplace_back([this]() {
			this->worker();
			});
	}
}

ThreadPool::~ThreadPool()
{
	//����ֹͣ��ʶ
	{
		std::unique_lock<std::mutex>(mtx); 
		isstop = true;		
	}
	//֪ͨ�������������е��߳�
	cv.notify_all();

	//ȷ���߳�ִ�����
	for (std::thread& onethread : workers)
	{
		onethread.join();
	}
}

template<typename F, typename ...Arg>
auto ThreadPool::enques(F&& f, Arg && ...arg)->std::future<typename std::result_of<F(Arg ...)>::type>
{
	//���fִ�к������
	using functype = typename std::result_of<F(Arg...)>::type;
	//���һ������ָ�� ָ��һ������װΪfunctype()��task
	auto task = std::make_shared<std::packaged_task<functype()>>(
		std::bind(std::forward<F>(f), std::forward<Arg>(arg)...)
	);
	//���future
	std::future<functype>rsfuture = task->get_future();
	//��������ӵ�����
	{
		std::lock_guard<std::mutex>lockguard(this->mtx);
		if (isstop)
			throw std::runtime_error("����:�̳߳��Ѿ�ֹͣ��");
		//��������ӵ�����
		myque.emplace([task]() {
			(*task)();
			});
	}
	//֪ͨ�߳�ȥִ������
	cv.notify_one();
	//����future
	return rsfuture;
	
}
//ÿ������Ĺ�������
void ThreadPool::worker()
{
	while (true)
	{
		//��������
		std::function<void()> task;
		//�Ӷ�����ȡ��һ������
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this] { return this->isstop | !this->myque.empty(); });
		if (isstop && myque.empty())
			return; 
		task = std::move(this->myque.front());
		this->myque.pop();
		//ִ������
		task();
	}
}