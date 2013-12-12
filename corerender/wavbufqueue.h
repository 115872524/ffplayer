#ifndef _WAVBUF_QUEUE_H_
#define _WAVBUF_QUEUE_H_

// ����ͷ�ļ�
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// ��������
#define DEF_WAVBUF_QUEUE_SIZE   32
#define DEF_WAVBUF_BUFFER_SIZE  4096

typedef struct {
    long      head;
    long      tail;
    long      size;
    long      curnum;
    HANDLE    semr;
    HANDLE    semw;
    WAVEHDR  *pwhdrs;
    HWAVEOUT  hwavout;
} WAVBUFQUEUE;

// ��������
BOOL wavbufqueue_create (WAVBUFQUEUE *pwq, HWAVEOUT h);
void wavbufqueue_destroy(WAVBUFQUEUE *pwq);
void wavbufqueue_flush  (WAVBUFQUEUE *pwq);
BOOL wavbufqueue_isempty(WAVBUFQUEUE *pwq);

//++ ���������ӿں������ڿ��п�д wavehdr �Ĺ��� ++//
// wavbufqueue_write_request ȡ�õ�ǰ��д�Ŀ��� wavehdr
// wavbufqueue_write_release �ͷŵ�ǰ��д�Ŀ��� wavehdr
// wavbufqueue_write_post ���д�����
// ����ʹ�÷�����
// wavbufqueue_write_request ���᷵�� wavehdr
// ��� wavehdr ������
// ������ɹ�����ִ��
//     wavbufqueue_write_post
// ������ʧ�ܣ���ִ��
//     wavbufqueue_write_release
void wavbufqueue_write_request(WAVBUFQUEUE *pwq, PWAVEHDR *pwhdr);
void wavbufqueue_write_release(WAVBUFQUEUE *pwq);
void wavbufqueue_write_done   (WAVBUFQUEUE *pwq);
//-- ���������ӿں������ڿ��п�д wavehdr �Ĺ��� --//

//++ ���������ӿں������ڿ��пɶ� wavehdr �Ĺ��� ++//
void wavbufqueue_read_request(WAVBUFQUEUE *pwq, PWAVEHDR *pwhdr);
void wavbufqueue_read_release(WAVBUFQUEUE *pwq);
void wavbufqueue_read_done   (WAVBUFQUEUE *pwq);
//-- ���������ӿں������ڿ��пɶ� wavehdr �Ĺ��� --//

#ifdef __cplusplus
}
#endif

#endif




