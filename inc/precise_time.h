//
// Created by Dandelonium on 7/4/2021.
//

#ifndef LAB_PARALLEL_GREP_PRECISE_TIME_H
#define LAB_PARALLEL_GREP_PRECISE_TIME_H

inline std::chrono::high_resolution_clock::time_point get_current_time_fenced()
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto res_time = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return res_time;
}

template<class D>
inline long long to_us(const D& d)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

#endif //LAB_PARALLEL_GREP_PRECISE_TIME_H
