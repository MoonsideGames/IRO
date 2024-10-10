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

#ifndef IRO_IMAGE_H
#define IRO_IMAGE_H

#include <SDL3/SDL_stdinc.h>

#ifdef _WIN32
#define IROAPI  __declspec(dllexport)
#define IROCALL __cdecl
#else
#define IROAPI
#define IROCALL
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Image Read API */

/* Decodes image format data into raw image data.
 *
 * w:		    Filled with the width of the image.
 * h:		    Filled with the height of the image.
 * len:			Filled with the length of pixel data in bytes.
 *
 * Returns a block of raw image memory suitable for use with graphics libraries.
 * Be sure to free the memory with IRO_FreeImage after use!
 */
IROAPI void *IRO_LoadImage(
    void *bufferPtr,
    Uint32 bufferLength,
    Uint32 *w,
    Uint32 *h,
    Uint32 *len);

/* Get image dimensions without fully decoding the image.
 *
 * w:		    Filled with the width of the image.
 * h:		    Filled with the height of the image.
 * len:			Filled with the length of pixel data in bytes.
 */
IROAPI bool IRO_GetImageInfo(
    void *bufferPtr,
    Uint32 bufferLength,
    Uint32 *w,
    Uint32 *h,
    Uint32 *len);

/* Frees memory returned by IRO_Image_Load. Do NOT free the memory yourself!
 *
 * mem: A pointer previously returned by IRO_Image_LoadPNG.
 */
IROAPI void IRO_FreeImage(void *mem);

/* Image Write API */

typedef void (IROCALL * IRO_WriteFunc)(
	void* context,
	void* data,
	int32_t size
);

/* Returns a buffer of PNG encoded from RGBA8 color data.
 *
 * data:	The raw color data.
 * w:		The width of the color data.
 * h:		The height of the color data.
 */
IROAPI bool IRO_EncodePNG(
    IRO_WriteFunc writeFunc,
    void *context,
    void *data,
    Uint32 w,
    Uint32 h);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* IRO_IMAGE_H */
