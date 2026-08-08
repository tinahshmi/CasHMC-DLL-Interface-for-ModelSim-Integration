#ifndef MEMORYAPI_H
#define MEMORYAPI_H

#include <fstream>
#include <iostream>
#include <cstdint> 
#include <queue>
#include <unordered_map>


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

class MemoryAPI : public DualVectorObject<Packet, Packet>
{     
private:
    struct OutstandingRequest
    {
        uint64_t address;
        unsigned bytes;
        bool write;
    };

    std::queue<Packet*> responseQueue;
    std::unordered_map<unsigned, OutstandingRequest> outstandingRequests;
    
    VaultController *vaultController;
    DRAM *dram;
    
    bool Send(Packet *packet);
public:
        MemoryAPI(std::ofstream &debugOut,std::ofstream &stateOut);
        virtual ~MemoryAPI();
        void Update();
        virtual void CallbackReceiveDown(Packet *packet, bool chkReceive);
        virtual void CallbackReceiveUp(Packet *packet, bool chkReceive);
        bool Reset();
        bool Read(uint64_t address, unsigned bytes);
        bool Write(uint64_t address,unsigned bytes);
        bool HasResponse() const;
        bool GetResponse(MemoryResponse &rsp);
};

}

#endif