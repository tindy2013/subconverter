#ifndef STL_EXTRA_H_INCLUDED
#define STL_EXTRA_H_INCLUDED

#include <vector>

template <typename T> inline void eraseElements(std::vector<T> &target)
{
    target.clear();
    target.shrink_to_fit();
}

template <typename T> inline void eraseElements(T &target)
{
    T().swap(target);
}

#endif // STL_EXTRA_H_INCLUDED
