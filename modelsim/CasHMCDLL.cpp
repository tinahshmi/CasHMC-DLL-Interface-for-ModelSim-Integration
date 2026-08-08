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

    ReadIniFile("ConfigSim.ini");
    ReadIniFile("ConfigDRAM.ini");

    debugOut.open("debug.log");
    stateOut.open("state.log");

    gMemory = new MemoryAPI(
            debugOut,
            stateOut);

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

bool HMC_Read(uint64_t address,unsigned bytes)
{
    if(!gMemory)
        return false;

    return gMemory->Read(address, bytes);
}

bool HMC_Write(uint64_t address,unsigned bytes)
{
    if(!gMemory)
        return false;

    return gMemory->Write(address, bytes);
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