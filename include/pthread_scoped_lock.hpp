#pragma once

#include <pthread.h>

namespace chat_detail
{
    class PthreadLock {
    public:
        explicit PthreadLock(pthread_mutex_t* m) noexcept
            :m_(m)
        {
            pthread_mutex_lock(m_);
        }

        ~PthreadLock()
        {
            pthread_mutex_unlock(m_);
        }

        PthreadLock(const PthreadLock&) = delete;
        PthreadLock& operator=(const PthreadLock&) = delete;
    private:
        pthread_mutex_t* m_;
    };
}