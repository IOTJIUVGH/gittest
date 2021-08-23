#ifndef __PHILIPS_LOG_H__
#define __PHILIPS_LOG_H__

extern int mxos_debug_enabled;
extern mos_mutex_id_t stdio_tx_mutex;

/***
#define philips_custom_log(N, M, ...)                                                              \
    do                                                                                             \
    {                                                                                              \
        if (mxos_debug_enabled == 0)                                                               \
            break;                                                                                 \
        mos_mutex_lock(stdio_tx_mutex);                                                            \
        printf("[%ld][%s:%4d] " M "\r\n", mos_time(), N, __LINE__, ##__VA_ARGS__);                 \
        mos_mutex_unlock(stdio_tx_mutex);                                                          \
    } while (0 == 1)
    ***/
   
#define philips_custom_log(N, M, ...)                                                               \
    do                                                                                              \
    {                                                                                               \
        if (mxos_debug_enabled == 0)                                                                \
            break;                                                                                  \
        mos_mutex_lock(stdio_tx_mutex);                                                             \
        printf("[%d] " M "\r\n", __LINE__, ##__VA_ARGS__);                                         \
        mos_mutex_unlock(stdio_tx_mutex);                                                           \
    } while (0 == 1)

#endif

/*
#define philips_custom_log(N, M, ...)                                                               \
    do                                                                                              \
    {                                                                                               \
        if (mxos_debug_enabled == 0)                                                                \
            break;                                                                                  \
        mos_mutex_lock(stdio_tx_mutex);                                                             \
        printf(M "\r\n", __LINE__, ##__VA_ARGS__);                                                  \
        mos_mutex_unlock(stdio_tx_mutex);                                                           \
    } while (0 == 1)

#endif
*/