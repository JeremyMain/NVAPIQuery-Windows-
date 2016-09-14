/*
 * MIT License
 * 
 * Copyright (c) 2016 Jeremy Main (jmain.jp@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// nvapiquery.cpp : Demonstrate how to load NVAPI and query GPU utilization metrics

#include <Windows.h>
#include <stdio.h>

// The NVAPI.h header file can be downloaded as part of the NVAPI SDK from the NVIDIA web site
// ref: https://developer.nvidia.com/nvapi
//
// After downloading and extracting the SDK contents, copy the header files, 
// "amd64" and "x86" lib folder contents to the "nvapi" folder
// 
// nvapiquery
//  „¤„Ÿnvapi
//     „¥„Ÿamd64
//     „¤„Ÿx86

#include "./nvapi/nvapi.h"

// Applications that use NVAPI must staticly link to the library and then on the first call 
// to NvAPI_Initialize, NVAPI DLL will then be loaded dynamically
#ifdef  _WIN64
    #pragma comment(lib, ".\\NVAPI\\amd64\\nvapi64.lib")
#else
    #pragma comment(lib, ".\\NVAPI\\x86\\nvapi.lib")
#endif

#define NVAPIQUERY_UTILIZATION_TYPE_GPU 0


// display information about the calling function and related error
void ShowErrorDetails(const NvAPI_Status nvRetVal, const char* pFunctionName)
{
    switch (nvRetVal)
    {
        case NVAPI_ERROR:
            printf("[%s] ERROR: NVAPI_ERROR", pFunctionName) ;	
            break ;
        case NVAPI_INVALID_ARGUMENT:
            printf("[%s] ERROR: NVAPI_INVALID_ARGUMENT - pDynamicPstatesInfo is NULL ", pFunctionName) ;	
            break ;
        case NVAPI_HANDLE_INVALIDATED:
            printf("[%s] ERROR: NVAPI_HANDLE_INVALIDATED", pFunctionName) ;	
            break ;
        case NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE:
            printf("[%s] ERROR: NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE", pFunctionName) ;	
            break ;
        case NVAPI_INCOMPATIBLE_STRUCT_VERSION:
            printf("[%s] ERROR: NVAPI_INCOMPATIBLE_STRUCT_VERSION", pFunctionName) ;	
            break ;
        case NVAPI_NOT_SUPPORTED:
            printf("[%s] ERROR: NVAPI_NOT_SUPPORTED", pFunctionName) ;	
            break ;
        default:
            printf("[%s] ERROR: 0x%x", nvRetVal) ;	
            break ;
    }
}

// Application entry point
int main(int argc, char* argv[])
{
    int iRetValue = -1 ;
    NvAPI_Status nvRetValue = NVAPI_ERROR ;

    // Before any of the NVAPI functions can be used NvAPI_Initialize() must be called
    nvRetValue = NvAPI_Initialize() ;
	if (NVAPI_OK != nvRetValue)
    {
        ShowErrorDetails(nvRetValue, "NvAPI_Initialize") ;
        return iRetValue ;
    }

    // Now that NVAPI has been initalized, before exiting normally or when handling 
    // an error condition, ensure NvAPI_Unload() is called

    // For each of the GPUs detected by NVAPI, query the device name, GPU, GPU memory, video encoder and decoder utilization

    // Get the number of GPUs and actual GPU handles
    NvU32 uiNumGPUs = 0 ;
    NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS];
    nvRetValue = NvAPI_EnumPhysicalGPUs(nvGPUHandle, &uiNumGPUs) ;
	if (NVAPI_OK != nvRetValue)
    {
        ShowErrorDetails(nvRetValue, "NvAPI_EnumPhysicalGPUs") ;
        NvAPI_Unload() ;
        return iRetValue ;
    }

    // In the case that no GPUs were detected
    if (0 == uiNumGPUs)
    {
        printf("No NVIDIA GPUs were detected.\r\n") ;
        NvAPI_Unload() ;
        return iRetValue ;
    }

    // Flag to denote unsupported queries (i.e. vGPU utilization in GRID software releases before August 2016)
    bool bGPUUtilSupported = true ;

    // Print out a header for the utilization output
    printf("Device #, Name, GPU(%%), Frame Buffer(%%), Video Encode(%%), Video Decode(%%)\r\n") ;

    // Iterate through all of the GPUs
    for (unsigned int iDevIDX = 0; iDevIDX < uiNumGPUs; iDevIDX++)
    {
        // Get the device name
        NvAPI_ShortString cDevicename = {'\0'} ;
        nvRetValue = NvAPI_GPU_GetFullName(nvGPUHandle[iDevIDX], cDevicename) ;
		if (NVAPI_OK != nvRetValue)
		{
			ShowErrorDetails(nvRetValue, "NvAPI_GPU_GetFullName") ;
	        NvAPI_Unload() ;
			return iRetValue ;
		}

        // Get the GPU Utilizaiton
        double dGPUUtilization = 0.0 ;
        NV_GPU_DYNAMIC_PSTATES_INFO_EX GPUperf;
		GPUperf.version =  MAKE_NVAPI_VERSION(NV_GPU_DYNAMIC_PSTATES_INFO_EX,1) ;
        nvRetValue = NvAPI_GPU_GetDynamicPstatesInfoEx(nvGPUHandle[iDevIDX], &GPUperf) ; 
		if (NVAPI_OK != nvRetValue)
		{
			ShowErrorDetails(nvRetValue, "NvAPI_GPU_GetDynamicPstatesInfoEx") ;
	        NvAPI_Unload() ;
			return iRetValue ;
		}

        // Confirm if this counter is enabled for the card / environment
        if(GPUperf.utilization[NVAPIQUERY_UTILIZATION_TYPE_GPU].bIsPresent)
        {
            dGPUUtilization = GPUperf.utilization[NVAPIQUERY_UTILIZATION_TYPE_GPU].percentage ;
        }
        else
        {
            bGPUUtilSupported = false ;
        }

        // Get the GPU frame buffer memory information
        NV_DISPLAY_DRIVER_MEMORY_INFO DeviceMemoryStatus ;
        DeviceMemoryStatus.version =  MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_MEMORY_INFO_V2,2) ;
        nvRetValue = NvAPI_GPU_GetMemoryInfo(nvGPUHandle[iDevIDX], (NV_DISPLAY_DRIVER_MEMORY_INFO*)&DeviceMemoryStatus);
		if (NVAPI_OK != nvRetValue)
		{
			ShowErrorDetails(nvRetValue, "NvAPI_GPU_GetMemoryInfo") ;
	        NvAPI_Unload() ;
			return iRetValue ;
		}

        // compute the amount of frame buffer memory that has been used
        unsigned int uiFrameBufferUsedKBytes = 0u;
		unsigned int uiFrameBufferTotalKBytes = 0u;
		uiFrameBufferUsedKBytes = DeviceMemoryStatus.dedicatedVideoMemory - DeviceMemoryStatus.curAvailableDedicatedVideoMemory ;
        uiFrameBufferTotalKBytes = DeviceMemoryStatus.dedicatedVideoMemory ;

        double dMemUtilzation = (((double)uiFrameBufferUsedKBytes / (double)uiFrameBufferTotalKBytes) * 100.0) ;        

        // Get the video encoder utilization (where supported)
        // NOTE: not supported with NVAPI

        // Get the video decoder utilization (where supported)
        // NOTE: not supported with NVAPI

        if (!bGPUUtilSupported)
        {
            printf("Device %d, %s, -, %.0f, -, -\r\n",  iDevIDX,  cDevicename, dMemUtilzation) ;
        }
        else
        {
            printf("Device %d, %s, %.0f, %.0f, -, -\r\n",  iDevIDX,  cDevicename, dGPUUtilization, dMemUtilzation) ;
        }
    }

	// unload NVAPI use
	NvAPI_Unload() ;
	iRetValue = 0 ;
	
    return iRetValue;
}

