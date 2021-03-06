/************************************************************************************
*  MV_BufferSaver.h First Bulid20140116 By tank@microview.com.cn
*  -------------------------------------------------------------------------------
*  微视图像千兆网相机BUF存图 头文件
*  -------------------------------------------------------------------------------
*  Copyright © 2009-2014 Microview Science & Technology Co.,Ltd .All Right Reserved 
************************************************************************************/

#ifdef __cplusplus
#define MVCAPI extern "C" __declspec (dllexport)
#else
#define MVCAPI __declspec (dllexport)
#endif


enum MV_FILE_TYPE
{
	IMG_FILE_RAW = 0,
	IMG_FILE_BMP = 1,
	IMG_FILE_JPG = 2,
	IMG_FILE_PNG = 3,
	IMG_FILE_TIF = 4,
	IMG_FILE_GIF = 5,
};

/*
 * @Parameters
 * 	[in ] lBuffer   input buffer pointer
 * 	[in ] lSize     input buffer size
 * 	[out] lWidth    output image width  
 * 	[out] lHeight   output image height  
 * 	[out] lBPP      output image bytes per pixel    
 * 	[in ] lFilePath output image save path  
 * 	[in ] lFormat   output image save format
 * 	[in ] lQuality  output image save quality
 *
 * @Return
 *	 TRUE if successful
*/

MVCAPI BOOL WINAPI MVBUF_SaveImage(
	IN PBYTE lBuffer,
	IN int lSize,
	OUT int lWidth, 
	OUT int lHeight, 
	OUT int lBPP, 
	IN PCHAR lFilePath, 
	IN MV_FILE_TYPE lFormat=IMG_FILE_RAW);

MVCAPI BOOL WINAPI MVBUF_SaveImageEx(
	IN PBYTE lBuffer, 
	IN int lSize, 
	OUT int lWidth, 
	OUT int lHeight, 
	OUT int lBPP, 
	IN PCHAR lFilePath, 
	IN MV_FILE_TYPE lFormat=IMG_FILE_RAW, 
	IN int lQuality=75);