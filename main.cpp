#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>

template <typename T>
class ring_buffer
{
public:
    ring_buffer(size_t capacity):
        storage(capacity + 1),
        tail(0),
        head(0)
    {}

    bool push(T value)
    {
        if (get_next(tail) == head)
        {
            return false;
        }

        storage[tail] = std::move(value);
        tail=get_next(tail);

        return true;
    }

    bool pop(T &value)
    {
        if (head == tail)
        {
            return false;
        }

        value = std::move(storage[head]);
        head=get_next(head);

        return true;
    }

private:
    size_t get_next(size_t slot) const
    {
        return (slot + 1) % storage.size();
    }

private:
    std::vector<T> storage;
    size_t tail;
    size_t head;
};

void test()
{
    int count = 10000000;

    ring_buffer<int> buffer(1024);

    auto start = std::chrono::steady_clock::now();

    std::thread producer([&]() {
        for (int i = 0; i < count; ++i)
        {
            while (!buffer.push(i))
            {
                std::this_thread::yield();
            }
        }
    });

    uint64_t sum = 0;

    std::thread consumer([&]() {
        for (int i = 0; i < count; ++i)
        {
            int value;

            while (!buffer.pop(value))
            {
                std::this_thread::yield();
            }

            sum += value;
        }
    });

    producer.join();
    consumer.join();

    auto finish = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "time: " << ms << "ms ";
    std::cout << "sum: " << sum << std::endl;
}

int main()
{
    while (true)
    {
        test();
    }

    return 0;
};


//g++ -std=c++17 -O2 -pthread main.cpp
//g++ -std=c++17 -O2 -pthread -fsanitize=thread main.cpp
