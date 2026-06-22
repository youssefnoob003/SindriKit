#include <sindri/common/buffer.h>
#include <windows.h>

void snd_buffer_init(snd_buffer_t *buf, LPVOID data, SIZE_T size, snd_free_cb free_routine) {
    if (!buf)
        return;

    buf->data         = data;
    buf->size         = size;
    buf->free_routine = free_routine;
}

void snd_buffer_free(snd_buffer_t *buf) {
    if (!buf)
        return;

    if (buf->data && buf->free_routine) {
        buf->free_routine(buf);
    }

    buf->data         = NULL;
    buf->size         = 0;
    buf->free_routine = NULL;
}
