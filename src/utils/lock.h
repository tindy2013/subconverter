#ifndef LOCK_H_INCLUDED
#define LOCK_H_INCLUDED

#include <atomic>
#include <thread>

class RWLock
{
    static constexpr int WRITE_LOCK_STATUS = -1;
    static constexpr int FREE_STATUS = 0;
private:
    const std::thread::id NULL_THREAD;
    const bool WRITE_FIRST;
    std::thread::id m_write_thread_id;
    std::atomic_int m_lockCount;
    std::atomic_uint m_writeWaitCount;
public:
    RWLock(const RWLock&) = delete;
    RWLock& operator=(const RWLock&) = delete;
    explicit RWLock(bool writeFirst = true): WRITE_FIRST(writeFirst), m_write_thread_id(), m_lockCount(0), m_writeWaitCount(0) {}
    virtual ~RWLock() = default;
    int readLock()
    {
        if (std::this_thread::get_id() != m_write_thread_id)
        {
            int count;
            if (WRITE_FIRST)
            {
                do
                {
                    while ((count = m_lockCount) == WRITE_LOCK_STATUS || m_writeWaitCount > 0);
                }
                while (!m_lockCount.compare_exchange_weak(count, count + 1));
            }
            else
            {
                do
                {
                    while ((count = m_lockCount) == WRITE_LOCK_STATUS);
                }
                while (!m_lockCount.compare_exchange_weak(count, count + 1));
            }
        }
        return m_lockCount;
    }
    int readUnlock()
    {
        if (std::this_thread::get_id() != m_write_thread_id)
            --m_lockCount;
        return m_lockCount;
    }
    int writeLock()
    {
        if (std::this_thread::get_id() != m_write_thread_id)
        {
            ++m_writeWaitCount;
            for (int zero = FREE_STATUS; !m_lockCount.compare_exchange_weak(zero, WRITE_LOCK_STATUS); zero = FREE_STATUS);
            --m_writeWaitCount;
            m_write_thread_id = std::this_thread::get_id();
        }
        return m_lockCount;
    }
    int writeUnlock()
    {
        if (std::this_thread::get_id() != m_write_thread_id)
        {
            throw std::runtime_error("writeLock/Unlock mismatch");
        }
        if (WRITE_LOCK_STATUS != m_lockCount)
        {
            throw std::runtime_error("RWLock internal error");
        }
        m_write_thread_id = NULL_THREAD;
        m_lockCount.store(FREE_STATUS);
        return m_lockCount;
    }
};

#endif //LOCK_H_INCLUDED
