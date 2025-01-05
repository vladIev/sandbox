#include <atomic>
#include <condition_variable>
#include <format>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

using MessageType = std::string;

class Consumer
{
	void handleMessage(const MessageType& msg) { }

	void handleMessage(MessageType&& msg) { }

  public:
	template <typename T>
	void consume(T&& t)
	{
		handleMessage(std::forward<T>(t));
	}
};

class Worker
{
	using Task = std::function<void()>;

	std::queue<Task> d_tasks;
	std::mutex d_mutex;
	std::atomic_bool d_isStopped;
	std::condition_variable d_hasTask;
	std::thread d_workerThread;

	void loop()
	{
		while(true)
		{
			Task task;
			{
				std::unique_lock lock(d_mutex);
				d_hasTask.wait(lock, [&]() { return !d_tasks.empty() || d_isStopped; });

				if(d_isStopped && d_tasks.empty())
				{
					break;
				}

				task = d_tasks.front();
				d_tasks.pop();
			}

			if(task)
			{
				try
				{
					task();
				}
				catch(const std::exception& e)
				{
					std::cerr << "Exception: " << e.what();
				}
				catch(...)
				{
					std::cerr << "Unknown exception: " << e.what();
				}
			}
		}
	}

  public:
	Worker()
		: d_isStopped(false)
		, d_workerThread(&Worker::loop, this)
	{ }

	~Worker()
	{
		if(d_workerThread.joinable())
		{
			d_workerThread.join();
		}
	}

	void enqueTask(Task task)
	{
		{
			std::lock_guard lock(d_mutex);
			d_tasks.emplace(std::move(task));
		}
		d_hasTask.notify_one();
	}

	void stop()
	{
		d_isStopped.store(true);
		d_hasTask.notify_all();
	}
};

class AsyncMessageDispatcher
{
  public:
	bool subscribe(int channelId, std::shared_ptr<Consumer> consumer);
	bool unsubscribe(int channelId);
	void dispatchMessage(int channelId, MessageType message)
};

auto main(int argc, char** argv) -> int
{
	Consumer consumer;
	consumer.consume("Message1");
	MessageType msg = "Message2";
	consumer.consume(msg);
	consumer.consume(std::move(msg));

	return 0;
}