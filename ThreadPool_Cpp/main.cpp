#include "ThreadPool.hpp"
#include <iostream>

int main()
{
	ThreadPool mypool(4);
	for (size_t i = 0; i < 20; i++)
	{
		auto rsfuture = mypool.enques([](int a, int b)->int {
			std::cout << "当先线程:" << std::this_thread::get_id() << std::endl;
			return a + b; }, 10 * i, 10 * i);
		std::cout << "thread rs:" << rsfuture.get() << std::endl;

	}
	return 0;
}