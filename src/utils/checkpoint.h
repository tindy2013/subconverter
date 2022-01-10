#ifndef CHECKPOINT_H_INCLUDED
#define CHECKPOINT_H_INCLUDED

#include <chrono>
#include <iostream>

inline std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> start_time;

inline void checkpoint()
{
    if(start_time == std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>())
        start_time = std::chrono::steady_clock::now();
    else
    {
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        std::cerr<<duration.count()<<"\n";
        start_time = end_time;
    }
}

#endif // CHECKPOINT_H_INCLUDED
