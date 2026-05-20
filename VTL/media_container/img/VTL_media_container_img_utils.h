#ifndef _VTL_IMG_UTILS_H
#define _VTL_IMG_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/img/VTL_media_container_img_data.h>

int VTL_img_CheckFileExists(const char* path);
int VTL_img_GetFileSize(const char* path);

int VTL_img_IsFormatSupported(const char* format);
const char* VTL_img_GetFormatDescription(const char* format);

#ifdef __cplusplus
}
#endif

#endif // _VTL_IMG_UTILS_H 