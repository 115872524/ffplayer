#ifndef _FFPLAYER_LOG_H_
#define _FFPLAYER_LOG_H_

// ����ͷ�ļ�
#include "stdefine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* �������� */
void log_init  (TCHAR *file);
void log_done  (void);
void log_printf(TCHAR *format, ...);

#ifdef __cplusplus
}
#endif

#endif
