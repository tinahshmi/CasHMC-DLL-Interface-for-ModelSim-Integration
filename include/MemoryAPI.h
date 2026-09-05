#ifndef MEMORYAPI_H
#define MEMORYAPI_H

#include <fstream>
#include <iostream>
#include <cstdint> 
#include <queue>
#include <unordered_map>
#include <vector>

#include "DualVectorObject.h"
#include "VaultController.h"
#include "DRAM.h"
#include "Packet.h"

namespace CasHMC
{
struct MemoryRequest
{
    bool write; // true = write, false = read

    uint64_t address;

    unsigned bytes;

    uint64_t *data;
};

struct MemoryResponse
{
    bool valid;

    bool writeAck;

    uint16_t tag;

    uint64_t address;

    unsigned bytes;

    // uint64_t *data; // nullptr in Version (timing-only model)
};
constexpr unsigned MEMORY_API_NUM_VAULTS = 16;
constexpr unsigned MEMORY_API_BUFFER_SIZE = 16 * 9;
class MemoryAPI : public DualVectorObject<Packet, Packet>
{     
private:
    struct OutstandingRequest
    {
        uint64_t address;
        unsigned bytes;
        bool write;
        unsigned vaultID;
    };

    std::queue<Packet*> responseQueue;
    std::unordered_map<unsigned, OutstandingRequest> outstandingRequests;
    
    std::vector<VaultController *> vaultControllers;
    std::vector<DRAM *> drams;
    
    bool Send(Packet *packet);
public:
        MemoryAPI(std::ofstream &debugOut,std::ofstream &stateOut);
        virtual ~MemoryAPI();
        void Update();
        virtual void CallbackReceiveDown(Packet *packet, bool chkReceive);
        virtual void CallbackReceiveUp(Packet *packet, bool chkReceive);
        bool Reset();
        bool Read(unsigned vaultID, uint64_t address, unsigned bytes);
        bool Write(unsigned vaultID, uint64_t address, unsigned bytes);
        bool HasResponse() const;
        bool GetResponse(MemoryResponse &rsp);
};

}

#endif