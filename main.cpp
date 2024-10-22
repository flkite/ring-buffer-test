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
        capacity(capacity),
        tail(0),
        head(0),
        curr_tail_pop(0),
        curr_head_pop(0),
        curr_tail_push(0),
        curr_head_push(0)
    {}

    bool push(T value)
    {
        size_t next=get_next(curr_tail_pop);
        if (next == curr_head_pop)
        {
            curr_head_pop = head.load();
            if (next == curr_head_pop)
                return false;
        }

        storage[curr_tail_pop] = std::move(value);
        curr_tail_pop=next;
        if((curr_tail_pop & 0x3)==0)
            tail.store(curr_tail_pop);

        return true;
    }

    bool pop(T &value)
    {
        if (curr_head_push == curr_tail_push)
        {
            curr_tail_push = tail.load();
            if (curr_head_push == curr_tail_push)
                return false;
        }

        value = std::move(storage[curr_head_push]);
        curr_head_push=get_next(curr_head_push);
        if((curr_head_push & 0x3)==0)
            head.store(curr_head_push);

        return true;
    }

private:
    size_t get_next(size_t slot) const
    {
        return slot<capacity?(slot+1):0;
    }

private:
    std::vector<T> storage;
    size_t capacity;
    std::atomic<size_t> tail;
    std::atomic<size_t> head;
    size_t curr_tail_pop;
    size_t curr_head_pop;
    size_t curr_tail_push;
    size_t curr_head_push;
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
