/*
 * soundmp3.c - Implementation of the MP3 dump sound device
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifdef USE_LAMEMP3

#include "lamelib.h"
#include "sound.h"
#include "types.h"
#include "archdep.h"
#include "lib.h"
#include "log.h"

/* HACK: Massive fixed size buffer for now, as the sound.c buffer has been made dynamic in size there is no more constant to use here. */
#define PCM_BUFFER_SIZE (SOUND_OUTPUT_CHANNELS_MAX * 1024 * 1024)
#define MP3_BUFFER_SIZE (PCM_BUFFER_SIZE + (PCM_BUFFER_SIZE / 4) + 7200)

static FILE *mp3_fd = NULL;
static int stereo = 0;
static int16_t *pcm_buffer = NULL;
static unsigned char *mp3_buffer = NULL;
static lame_global_flags *gfp;

static int mp3_init(const char *param, int *speed, int *fragsize, int *fragnr, int *channels)
{
    mp3_fd = fopen(param ? param : "vicesnd.mp3", MODE_WRITE);
    if (!mp3_fd) {
        return 1;
    }

    gfp = vice_lame_init();
    vice_lame_set_num_channels(gfp, *channels);
    vice_lame_set_in_samplerate(gfp, *speed);
    vice_lame_set_brate(gfp, 128);
    vice_lame_set_quality(gfp, 2);

    if (vice_lame_init_params(gfp) < 0) {
        vice_lame_close(gfp);
        fclose(mp3_fd);
        return 1;
    }

    if (pcm_buffer == NULL) {
        pcm_buffer = lib_malloc(sizeof(int16_t) * PCM_BUFFER_SIZE);
        if (pcm_buffer == NULL) {
            return 1;
        }
    }

    if (mp3_buffer == NULL) {
        mp3_buffer = lib_malloc(sizeof(int16_t) * MP3_BUFFER_SIZE);
        if (mp3_buffer == NULL) {
            return 1;
        }
    }

    if (*channels == 2) {
        stereo = 1;
    }

    return 0;
}

static int mp3_write(int16_t *pbuf, size_t nr)
{
    int mp3_size;
    unsigned int i;

    if (pcm_buffer == NULL) {
        return 1;
    }

    if (mp3_buffer == NULL) {
        return 1;
    }

    for (i = 0; i < nr; i++) {
        if (stereo == 1) {
            pcm_buffer[i] = pbuf[i];
        } else {
            pcm_buffer[i * 2] = pbuf[i];
            pcm_buffer[(i * 2) + 1] = pbuf[i];
        }
    }

    mp3_size = vice_lame_encode_buffer_interleaved(gfp, pcm_buffer,
            (int)((stereo == 1) ? nr / 2 : nr), mp3_buffer, MP3_BUFFER_SIZE);
    if (mp3_size != 0) {
        if (mp3_size != (int)fwrite(mp3_buffer, 1, (size_t)mp3_size, mp3_fd)) {
            return 1;
        }
    }
    return 0;
}

static void mp3_close(void)
{
    int mp3_size;

    mp3_size = vice_lame_encode_flush(gfp, mp3_buffer, MP3_BUFFER_SIZE);

    if (fwrite(mp3_buffer, 1, (size_t)mp3_size, mp3_fd) != (size_t)mp3_size) {
        log_debug(LOG_DEFAULT, "ERROR mp3_close failed.");
    }
    fclose(mp3_fd);
    mp3_fd = NULL;

    vice_lame_close(gfp);

    if (pcm_buffer != NULL) {
        lib_free(pcm_buffer);
        pcm_buffer = NULL;
    }

    if (mp3_buffer != NULL) {
        lib_free(mp3_buffer);
        mp3_buffer = NULL;
    }
}

static const sound_device_t mp3_device =
{
    "mp3",
    mp3_init,
    mp3_write,
    NULL,
    NULL,
    NULL,
    NULL,
    mp3_close,
    NULL,
    NULL,
    0,
    2,
    false
};

int sound_init_mp3_device(void)
{
#ifndef HAVE_STATIC_LAME
    int result = lamelib_open();
    if (result != 0) {
        log_debug(LOG_DEFAULT, "ERROR setting up dynamic lame lib!");
        return result;
    }
#endif

    return sound_register_device(&mp3_device);
}

#endif /* ifdef USE_LAMEMP3 */
