#ifndef _WAV_QUEUE_H_
#define _WAV_QUEUE_H_

// ����ͷ�ļ�
#include <inttypes.h>
#include <semaphore.h>

// ��������
#define DEF_WAV_QUEUE_SIZE   5
#define DEF_WAV_BUFFER_SIZE  8192

typedef struct {
    long      head;
    long      tail;
    long      size;
    sem_t     semr;
    sem_t     semw;
    int64_t  *ppts;
} WAVQUEUE;

// ��������
bool wavqueue_create (WAVQUEUE *pwq);
void wavqueue_destroy(WAVQUEUE *pwq);
bool wavqueue_isempty(WAVQUEUE *pwq);

//++ ���������ӿں������ڿ��п�д wavehdr �Ĺ��� ++//
// wavqueue_write_request ȡ�õ�ǰ��д�Ŀ��� wavehdr
// wavqueue_write_release �ͷŵ�ǰ��д�Ŀ��� wavehdr
// wavqueue_write_post ���д�����
// ����ʹ�÷�����
// wavqueue_write_request ���᷵�� wavehdr
// ��� wavehdr ������
// ������ɹ�����ִ��
//     wavqueue_write_post
// ������ʧ�ܣ���ִ��
//     wavqueue_write_release
void wavqueue_write_request(WAVQUEUE *pwq, int64_t **ppts);
void wavqueue_write_release(WAVQUEUE *pwq);
void wavqueue_write_done   (WAVQUEUE *pwq);
//-- ���������ӿں������ڿ��п�д wavehdr �Ĺ��� --//

//++ ���������ӿں������ڿ��пɶ� wavehdr �Ĺ��� ++//
void wavqueue_read_request(WAVQUEUE *pwq, int64_t **ppts);
void wavqueue_read_release(WAVQUEUE *pwq);
void wavqueue_read_done   (WAVQUEUE *pwq);
//-- ���������ӿں������ڿ��пɶ� wavehdr �Ĺ��� --//

#endif




