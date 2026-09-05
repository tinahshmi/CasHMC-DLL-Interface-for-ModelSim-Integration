#include "CasHMCDLL.h"

#include "MemoryAPI.h"
#include "ConfigReader.h"

#include <fstream>

using namespace CasHMC;

static MemoryAPI *gMemory = nullptr;

static std::ofstream debugOut;
static std::ofstream stateOut;

bool HMC_Init()
{
    if(gMemory)
        return true;

    printf("[DLL] HMC_Init: ENTER\n");
    fflush(stdout);

    printf("[DLL] Before ConfigSim\n");
    fflush(stdout);

    ReadIniFile("ConfigSim.ini");

    printf("[DLL] After ConfigSim\n");
    fflush(stdout);

    printf("[DLL] Before ConfigDRAM\n");
    fflush(stdout);

    ReadIniFile("ConfigDRAM.ini");

    printf("[DLL] After ConfigDRAM\n");
    fflush(stdout);

    printf("[DLL] Before debugOut.open\n");
    fflush(stdout);

    debugOut.open("debug.log");

    printf("[DLL] After debugOut.open\n");
    fflush(stdout);

    printf("[DLL] Before stateOut.open\n");
    fflush(stdout);

    stateOut.open("state.log");

    printf("[DLL] After stateOut.open\n");
    fflush(stdout);

    printf("[DLL] Before MemoryAPI constructor\n");
    fflush(stdout);

    gMemory = new MemoryAPI(
            debugOut,
            stateOut);

    printf("[DLL] After MemoryAPI constructor\n");
    fflush(stdout);

    printf("[DLL] HMC_Init: RETURN TRUE\n");
    fflush(stdout);

    return true;
}

void HMC_Shutdown()
{
    delete gMemory;

    gMemory = nullptr;

    debugOut.close();
    stateOut.close();
}

void HMC_Update()
{
    if(gMemory)
        gMemory->Update();
}

void HMC_Reset()
{
    if(gMemory)
        gMemory->Reset();
}

bool HMC_Read(
        unsigned vaultID,
        uint64_t address,
        unsigned bytes)
{
    if(!gMemory)
        return false;

    return gMemory->Read(
        vaultID,
        address,
        bytes);
}

bool HMC_Write(
        unsigned vaultID,
        uint64_t address,
        unsigned bytes)
{
    if(!gMemory)
        return false;

    return gMemory->Write(
        vaultID,
        address,
        bytes);
}

bool HMC_HasResponse()
{
    if(gMemory == nullptr)
        return false;

    return gMemory->HasResponse();
}

bool HMC_GetResponse(
        bool *writeAck,
        uint16_t *tag,
        uint64_t *address,
        unsigned *bytes)
{
    if(!gMemory)
        return false;
    
    if(writeAck == nullptr ||
       tag      == nullptr ||
       address  == nullptr ||
       bytes    == nullptr)
        return false;

    MemoryResponse rsp;

    if(!gMemory->GetResponse(rsp))
        return false;

    *writeAck = rsp.writeAck;
    *tag      = rsp.tag;
    *address  = rsp.address;
    *bytes    = rsp.bytes;

    return true;
}