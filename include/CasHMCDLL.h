#ifndef CASHMC_DLL_H
#define CASHMC_DLL_H

#include <stdint.h>

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

//-----------------------------------------------------
// Initialization
//-----------------------------------------------------

DLL_EXPORT bool HMC_Init();

DLL_EXPORT void HMC_Shutdown();

DLL_EXPORT void HMC_Update();

DLL_EXPORT void HMC_Reset();


//-----------------------------------------------------
// Requests
//-----------------------------------------------------

DLL_EXPORT bool HMC_Read(
        unsigned vaultID,
        uint64_t address,
        unsigned bytes);

DLL_EXPORT bool HMC_Write(
        unsigned vaultID,
        uint64_t address,
        unsigned bytes);


//-----------------------------------------------------
// Responses
//-----------------------------------------------------
DLL_EXPORT bool HMC_HasResponse();
DLL_EXPORT bool HMC_GetResponse(
        bool *writeAck,
        uint16_t *tag,
        uint64_t *address,
        unsigned *bytes);

#ifdef __cplusplus
}

#endif

#endif