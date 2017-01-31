#include "knobs.h"

int CKnobs::getValue() {
    int ret;
    pthread_mutex_lock(&m_mtx_value);
    ret = m_value;
    pthread_mutex_unlock(&m_mtx_value);
    return ret;
}

void CKnobs::setValue(int value) {
    pthread_mutex_lock(&m_mtx_value);
    m_value = value;
    pthread_mutex_unlock(&m_mtx_value);
}
