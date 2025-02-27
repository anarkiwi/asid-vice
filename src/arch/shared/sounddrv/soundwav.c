/*
 * soundwav.c - Implementation of the RIFF/WAV dump sound device
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Dag Lem <resid@nimrod.no>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "sound.h"
#include "types.h"
#include "archdep.h"
#include "log.h"

static FILE *wav_fd = NULL;
static int samples = 0;

/* Store number as little endian. */
static void le_store(uint8_t *buf, uint32_t val, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        buf[i] = (uint8_t)(val & 0xff);
        val >>= 8;
    }
}

static int wav_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    /* RIFF/WAV header. */
    unsigned char header[45] = "RIFFllllWAVEfmt \020\0\0\0\001\0ccrrrrbbbb88\020\0datallll";
    uint32_t sample_rate = (uint32_t)*speed;
    uint32_t bytes_per_sec = (uint32_t)(*speed * *channels * 2);

    wav_fd = fopen(param ? param : "vicesnd.wav", MODE_WRITE);
    if (!wav_fd) {
        return 1;
    }

    /* Reset number of samples. */
    samples = 0;

    /* Initialize header. */
    le_store(header + 22, (uint32_t)*channels, 2);
    le_store(header + 24, sample_rate, 4);
    le_store(header + 28, bytes_per_sec, 4);
    le_store(header + 32, (uint32_t)*channels * 2, 2);

    return (fwrite(header, 1, 44, wav_fd) != 44);
}

static int wav_write(int16_t *pbuf, size_t nr)
{
#ifdef WORDS_BIGENDIAN
    unsigned int i;

    /* Swap bytes on big endian machines. */
    for (i = 0; i < nr; i++) {
        pbuf[i] = (int16_t)((((uint16_t)pbuf[i] & 0xff) << 8) | ((uint16_t)pbuf[i] >> 8));
    }
#endif

    if (nr != fwrite(pbuf, sizeof(int16_t), nr, wav_fd)) {
        return 1;
    }

    /* Swap the bytes back just in case. */
#ifdef WORDS_BIGENDIAN
    for (i = 0; i < nr; i++) {
        pbuf[i] = (int16_t)((((uint16_t)pbuf[i] & 0xff) << 8) | ((uint16_t)pbuf[i] >> 8));
    }
#endif

    /* Accumulate number of samples. */
    samples += (int)nr;

    return 0;
}

static void wav_close(void)
{
    int res = -1;
    uint8_t rlen[4];
    uint8_t dlen[4];
    uint32_t rifflen = (uint32_t)(samples * 2 + 36);
    uint32_t datalen = (uint32_t)(samples * 2);

    le_store(rlen, rifflen, 4);
    le_store(dlen, datalen, 4);

    fseek(wav_fd, 4, SEEK_SET);
    if (fwrite(rlen, 1, 4, wav_fd) == 4) {
        fseek(wav_fd, 32, SEEK_CUR);
        if (fwrite(dlen, 1, 4, wav_fd) == 4) {
            res = 0;
        }
    }

    fclose(wav_fd);
    wav_fd = NULL;
    if (res != 0) {
        log_debug(LOG_DEFAULT, "ERROR wav_close failed.");
    }
}

static const sound_device_t wav_device =
{
    "wav",
    wav_init,
    wav_write,
    NULL,
    NULL,
    NULL,
    NULL,
    wav_close,
    NULL,
    NULL,
    0,
    2,
    false
};

int sound_init_wav_device(void)
{
    return sound_register_device(&wav_device);
}
