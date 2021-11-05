#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

class ThreadPool
{
public:
  ThreadPool(std::size_t number_of_threads);
  ~ThreadPool();

  template<class F, class... ArgTypes>
  auto add_task(F&& f, ArgTypes&&... args)
    -> std::future<typename std::invoke_result<F, ArgTypes...>::type>;

private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;

  std::mutex mutex;
  std::condition_variable condition_variable;
  bool stop;
};

inline ThreadPool::ThreadPool(std::size_t number_of_threads)
  : stop(false)
{
  for (std::size_t i = 0; i < number_of_threads; ++i) {
    workers.emplace_back(
      [this]()
      {
        for (;;) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->condition_variable.wait(lock, [this]() {
              return this->stop || !this->tasks.empty();
              });
            if (this->stop && this->tasks.empty()) {
              return;
            }
            task = std::move(this->tasks.front());
            this->tasks.pop();
          }
          task();
        }
      }
    );
  }
}

inline ThreadPool::~ThreadPool()
{
  {
    std::unique_lock<std::mutex> lock(mutex);
    stop = true;
  }
  condition_variable.notify_all();
  for (auto& worker : workers) {
    worker.join();
  }
}

template<class F, class... ArgTypes>
auto ThreadPool::add_task(F&& f, ArgTypes&&... args)
  ->std::future<typename std::invoke_result<F, ArgTypes...>::type>
{
  using return_type = typename std::invoke_result<F, ArgTypes...>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
    std::bind(std::forward<F>(f), std::forward<ArgTypes>(args)...)
  );

  std::future<return_type> result = task->get_future();
  {
    std::unique_lock<std::mutex> lock(mutex);

    if (stop) {
      throw std::runtime_error("Adding tasks after `stop` is not allowed.");
    }
    tasks.emplace([task]() { (*task)(); });
  }
  condition_variable.notify_one();
  return result;
}
