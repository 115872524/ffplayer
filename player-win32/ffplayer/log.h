#ifndef __FFPLAYER_LOG_H__
#define __FFPLAYER_LOG_H__

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
