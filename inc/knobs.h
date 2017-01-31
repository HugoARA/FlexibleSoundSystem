#ifndef KNOBS_H
#define KNOBS_H
#pragma once

#include <pthread.h>

class CKnobs {
    int m_value;
    pthread_mutex_t m_mtx_value;
public:
    CKnobs() : m_mtx_value(PTHREAD_MUTEX_INITIALIZER) {}
    int getValue();
    void setValue(int);
};

#endif // KNOBS_H
