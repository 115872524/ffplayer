// ����ͷ�ļ�
#include <pthread.h>
#include <semaphore.h>
#include <media/AudioRecord.h>
#include <media/AudioTrack.h>
#include "adev.h"

using namespace android;

// �ڲ���������
#define DEF_ADEV_BUF_NUM  8
#define DEF_ADEV_BUF_LEN  8192

#define SW_VOLUME_MINDB  -30
#define SW_VOLUME_MAXDB  +12

// �ڲ����Ͷ���
typedef struct
{
    sp<AudioTrack> audiotrack;

    int64_t  *ppts;
    AUDIOBUF *pWaveHdr;
    int       bufnum;
    int       buflen;
    int       head;
    int       tail;
    sem_t     semr;
    sem_t     semw;
    #define ADEV_CLOSE (1 << 0)
    #define ADEV_PAUSE (1 << 1)
    int       status;
    pthread_t thread;
    int64_t  *apts;

    // software volume
    int      vol_scaler[256];
    int      vol_zerodb;
    int      vol_curvol;
} ADEV_CONTEXT;

static void* audio_render_thread_proc(void *param)
{
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)param;
    while (!(c->status & ADEV_CLOSE))
    {
        if (c->status & ADEV_PAUSE) {
            usleep(10*1000);
            continue;
        }

        sem_wait(&c->semr);
        c->audiotrack->write(c->pWaveHdr[c->head].data, c->pWaveHdr[c->head].size);
        if (c->apts) *c->apts = c->ppts[c->head];
        if (++c->head == c->bufnum) c->head = 0;
        sem_post(&c->semw);
    }

    return NULL;
}

static int init_software_volmue_scaler(int *scaler, int mindb, int maxdb)
{
    double a[256];
    double b[256];
    int    z, i;

    for (i=0; i<256; i++) {
        a[i]      = mindb + (maxdb - mindb) * i / 256.0;
        b[i]      = pow(10.0, a[i] / 20.0);
        scaler[i] = (int)(0x10000 * b[i]);
    }

    z = -mindb * 256 / (maxdb - mindb);
    z = z > 0   ? z : 0  ;
    z = z < 255 ? z : 255;
    scaler[0] = 0;        // mute
    scaler[z] = 0x10000;  // 0db
    return z;
}

// �ӿں���ʵ��
void* adev_create(int type, int bufnum, int buflen)
{
    ADEV_CONTEXT *ctxt = NULL;
    uint8_t      *pwavbuf;
    int           i;

    // allocate adev context
    ctxt = new ADEV_CONTEXT();
    if (!ctxt) {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate adev context !\n");
        exit(0);
    }

    bufnum         = bufnum ? bufnum : DEF_ADEV_BUF_NUM;
    buflen         = buflen ? buflen : DEF_ADEV_BUF_LEN;
    ctxt->bufnum   = bufnum;
    ctxt->buflen   = buflen;
    ctxt->head     = 0;
    ctxt->tail     = 0;
    ctxt->ppts     = (int64_t *)calloc(bufnum, sizeof(int64_t));
    ctxt->pWaveHdr = (AUDIOBUF*)calloc(bufnum, (sizeof(AUDIOBUF) + buflen));

    // init wavebuf
    pwavbuf = (uint8_t*)(ctxt->pWaveHdr + bufnum);
    for (i=0; i<bufnum; i++) {
        ctxt->pWaveHdr[i].data = (int16_t*)(pwavbuf + i * buflen);
        ctxt->pWaveHdr[i].size = buflen;
    }

    // create AudioTrack
    ctxt->audiotrack = new AudioTrack(
        AUDIO_STREAM_DEFAULT,
        44100,
        AUDIO_FORMAT_PCM_16_BIT,
        AUDIO_CHANNEL_OUT_STEREO,
        ctxt->buflen);
    ctxt->audiotrack->start();

    // init software volume scaler
    ctxt->vol_zerodb = init_software_volmue_scaler(ctxt->vol_scaler, SW_VOLUME_MINDB, SW_VOLUME_MAXDB);
    ctxt->vol_curvol = ctxt->vol_zerodb;

    // create semaphore
    sem_init(&ctxt->semr, 0, 0     );
    sem_init(&ctxt->semw, 0, bufnum);

    // create audio rendering thread
    pthread_create(&ctxt->thread, NULL, audio_render_thread_proc, ctxt);

    return ctxt;
}

void adev_destroy(void *ctxt)
{
    if (!ctxt) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;

    // make audio rendering thread safely exit
    c->status = ADEV_CLOSE;
    sem_post(&c->semr);
    pthread_join(c->thread, NULL);

    // stop audiotrack
    c->audiotrack->stop();

    // close semaphore
    sem_destroy(&c->semr);
    sem_destroy(&c->semw);

    // free buffers
    free(c->ppts);
    free(c->pWaveHdr);

    // destroy audiotrack
    delete c;
}

void adev_request(void *ctxt, AUDIOBUF **ppab)
{
    if (!ctxt) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    sem_wait(&c->semw);
    *ppab = (AUDIOBUF*)&c->pWaveHdr[c->tail];
    (*ppab)->size = c->buflen;
}

void adev_post(void *ctxt, int64_t pts)
{
    if (!ctxt) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    c->ppts[c->tail] = pts;

    //++ software volume scale
    int      multiplier = c->vol_scaler[c->vol_curvol];
    int16_t *buf        = c->pWaveHdr[c->tail].data;
    int      n          = c->pWaveHdr[c->tail].size / sizeof(int16_t);
    if (multiplier > 0x10000) {
        int64_t v;
        while (n--) {
            v = ((int64_t)*buf * multiplier) >> 16;
            v = v < 0x7fff ? v : 0x7fff;
            v = v >-0x7fff ? v :-0x7fff;
            *buf++ = (int16_t)v;
        }
    }
    else if (multiplier < 0x10000) {
        while (n--) {
            *buf = ((int32_t)*buf * multiplier) >> 16; buf++;
        }
    }
    //-- software volume scale

    if (++c->tail == c->bufnum) c->tail = 0;
    sem_post(&c->semr);
}

void adev_pause(void *ctxt, int pause)
{
    if (!ctxt) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    if (pause) {
        c->status |=  ADEV_PAUSE;
        c->audiotrack->pause();
    }
    else {
        c->status &= ~ADEV_PAUSE;
        c->audiotrack->start();
    }
}

void adev_reset(void *ctxt)
{
    if (!ctxt) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    while (0 != sem_trywait(&c->semr)) {
        sem_post(&c->semw);
    }
    c->head   = 0;
    c->tail   = 0;
    c->status = 0;
    c->audiotrack->flush();
}

void adev_syncapts(void *ctxt, int64_t *apts)
{
    if (!ctxt) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    c->apts = apts;
    if (c->apts) {
        *c->apts = -1;
    }
}

void adev_curdata(void *ctxt, void **buf, int *len)
{
    if (!ctxt) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;
    if (buf) *buf = NULL;
    if (len) *len = 0;
}

void adev_setparam(void *ctxt, int id, void *param)
{
    if (!ctxt || !param) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;

    switch (id) {
    case PARAM_AUDIO_VOLUME:
        {
            int vol = *(int*)param;
            vol += c->vol_zerodb;
            vol  = vol > 0   ? vol : 0  ;
            vol  = vol < 255 ? vol : 255;
            c->vol_curvol = vol;
        }
        break;
    }
}

void adev_getparam(void *ctxt, int id, void *param)
{
    if (!ctxt || !param) return;
    ADEV_CONTEXT *c = (ADEV_CONTEXT*)ctxt;

    switch (id) {
    case PARAM_AUDIO_VOLUME:
        *(int*)param = c->vol_curvol - c->vol_zerodb;
        break;
    }
}

