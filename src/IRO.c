/* IRO - A simple cross-platform image loader library
 *
 * Copyright (c) 2024 Evan Hemsley
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Evan "cosmonaut" Hemsley <evan@moonside.games>
 *
 */

#include "IRO.h"
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_assert.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif

#ifndef __STDC_WANT_SECURE_LIB__
#define __STDC_WANT_SECURE_LIB__ 1
#endif
#define sprintf_s SDL_snprintf

#define abs    SDL_abs
#define ceilf  SDL_ceilf
#define floorf SDL_floorf
#define ldexp  SDL_scalbn
#define pow    SDL_pow

#ifdef memcmp
#undef memcmp
#endif
#define memcmp SDL_memcmp
#ifdef memcpy
#undef memcpy
#endif
#define memcpy SDL_memcpy
#ifdef memmove
#undef memmove
#endif
#define memmove SDL_memmove
#ifdef memset
#undef memset
#endif
#define memset SDL_memset
#ifdef strcmp
#undef strcmp
#endif
#define strcmp SDL_strcmp
#ifdef strlen
#undef strlen
#endif
#define strlen SDL_strlen

/* These are per the Texture2D.FromStream spec */
#define STBI_ONLY_PNG
#define STBI_ONLY_QOI

/* These are per the Texture2D.SaveAs* spec */
#define STBIW_ONLY_PNG
#define STBI_WRITE_NO_STDIO

static void *aligned_realloc(void *mem, const size_t len)
{
    const size_t alignment = SDL_GetSIMDAlignment();
    const size_t padding = alignment - (len % alignment);
    const size_t padded = (padding != alignment) ? (len + padding) : len;
    Uint8 *retval = (Uint8 *)mem;
    void *oldmem = mem;
    size_t memdiff, ptrdiff;
    Uint8 *ptr;

    if (mem) {
        void **realptr = (void **)mem;
        realptr--;
        mem = *(((void **)mem) - 1);

        /* Check the delta between the real pointer and user pointer */
        memdiff = ((size_t)oldmem) - ((size_t)mem);
    }

    ptr = (Uint8 *)SDL_realloc(mem, padded + alignment + sizeof(void *));

    if (ptr == mem) {
        return retval; /* Pointer didn't change, nothing to do */
    }
    if (ptr == NULL) {
        return NULL; /* Out of memory, bail! */
    }

    /* Store the actual malloc pointer right before our aligned pointer. */
    retval = ptr + sizeof(void *);
    retval += alignment - (((size_t)retval) % alignment);

    /* Make sure the delta is the same! */
    if (mem) {
        ptrdiff = ((size_t)retval) - ((size_t)ptr);
        if (memdiff != ptrdiff) { /* Delta has changed, copy to new offset! */
            oldmem = (void *)(((size_t)ptr) + memdiff);

            /* Even though the data past the old `len` is undefined, this is the
             * only length value we have, and it guarantees that we copy all the
             * previous memory anyhow.
             */
            SDL_memmove(retval, oldmem, len);
        }
    }

    /* Actually store the malloc pointer, finally. */
    *(((void **)retval) - 1) = ptr;
    return retval;
}

#define STB_IMAGE_STATIC
#define STBI_NO_HDR
#define STBI_NO_GIF
#define STBI_ASSERT  SDL_assert
#define STBI_MALLOC(size)  SDL_aligned_alloc(SDL_GetSIMDAlignment(), size)
#define STBI_REALLOC aligned_realloc
#define STBI_FREE    SDL_aligned_free
#define STB_IMAGE_IMPLEMENTATION
#ifdef __MINGW32__
#define STBI_NO_THREAD_LOCALS /* FIXME: Port to SDL_TLS -flibit */
#endif
#include "stb_image.h"

#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_SDL_MALLOC
#define MZ_ASSERT(x) SDL_assert(x)
#include "miniz.h"

/* Thanks Daniel Gibson! */
static unsigned char *dgibson_stbi_zlib_compress(
    unsigned char *data,
    int data_len,
    int *out_len,
    int quality)
{
    mz_ulong buflen = mz_compressBound(data_len);
    unsigned char *buf = SDL_malloc(buflen);

    if (buf == NULL ||
        mz_compress2(buf, &buflen, data, data_len, quality) != 0) {
        SDL_free(buf);
        return NULL;
    }
    *out_len = buflen;
    return buf;
}

static bool stbi_zlib_decompress(
    unsigned char *inBuf,
    unsigned char *outBuf,
    Uint32 in_len,
    Uint32 out_len)
{
    if (inBuf == NULL || outBuf == NULL) {
        return false;
    }

    mz_ulong out_mz = out_len;

    if (mz_uncompress(outBuf, &out_mz, inBuf, in_len) != 0) {
        return false;
    }

    return true;
}

#define STB_IMAGE_WRITE_STATIC
#define STBIW_ASSERT        SDL_assert
#define STBIW_MALLOC        SDL_malloc
#define STBIW_REALLOC       SDL_realloc
#define STBIW_FREE          SDL_free
#define STBIW_ZLIB_COMPRESS dgibson_stbi_zlib_compress
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#pragma GCC diagnostic pop

/* Image Read API */

void *IRO_LoadImage(
    void *bufferPtr,
    Uint32 bufferLength,
    Uint32 *w,
    Uint32 *h,
    Uint32 *len)
{
    Uint8 *result;
    Uint8 *pixels;
    Sint32 format;
    Sint32 i;
	int load_w;
	int load_h;

	*w = 0;
	*h = 0;
	*len = 0;

    result = stbi_load_from_memory(
        bufferPtr,
        bufferLength,
        &load_w,
        &load_h,
        &format,
        STBI_rgb_alpha);

    if (result == NULL) {
        SDL_LogWarn(SDL_LOG_CATEGORY_ERROR, "Image loading failed: %s", stbi_failure_reason());
		return NULL;
    }

	*w = load_w;
	*h = load_h;

    /* Ensure that the alpha pixels are... well, actual alpha.
     * You think this looks stupid, but be assured: Your paint program is
     * almost certainly even stupider.
     * -flibit
     */
    pixels = result;
    *len = (*w) * (*h) * 4;
    for (i = 0; i < *len; i += 4, pixels += 4) {
        if (pixels[3] == 0) {
            pixels[0] = 0;
            pixels[1] = 0;
            pixels[2] = 0;
        }
    }

    return result;
}

bool IRO_GetImageInfo(
    void *bufferPtr,
    Uint32 bufferLength,
    Uint32 *w,
    Uint32 *h,
    Uint32 *len)
{
    Sint32 format;
    Sint32 result;
	int load_w, load_h;

	*w = 0;
	*h = 0;
	*len = 0;

    result = stbi_info_from_memory(
        bufferPtr,
        bufferLength,
        &load_w,
        &load_h,
        &format);

    if (result == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Image info failed: %s", stbi_failure_reason());
		return false;
    }

	*w = load_w;
	*h = load_h;
    *len = (*w) * (*h) * 4;

    return result;
}

/* Image Write API */

void* IRO_EncodePNG(
    void *data,
    Uint32 w,
    Uint32 h,
    Sint32 *size)
{
    return stbi_write_png_to_mem(
        data,
        w * 4,
        w,
        h,
        4,
        size);
}

bool IRO_WritePNG(
    IRO_WriteFunc writeFunc,
    void *context,
    void *data,
    Uint32 w,
    Uint32 h)
{
    return stbi_write_png_to_func(
        writeFunc,
		context,
        w,
        h,
        4,
        data,
        w * 4);
}

/* Compression API */

void *IRO_Compress(
    void *data,
    Uint32 dataLength,
    int compressionLevel,
    Uint32 *outLength)
{
    int outlen;
    unsigned char *result = dgibson_stbi_zlib_compress(data, dataLength, &outlen, compressionLevel);
    *outLength = outlen;
    return result;
}

bool IRO_Decompress(
    void *encodedBuffer,
    void *decodedBuffer,
    Uint32 encodedLength,
    Uint32 decodedLength)
{
    return stbi_zlib_decompress(
        encodedBuffer,
        decodedBuffer,
        encodedLength,
        decodedLength);
}

void IRO_FreeBuffer(void *buffer)
{
    SDL_free(buffer);
}

void IRO_FreeImage(void *mem)
{
    SDL_aligned_free(mem);
}
