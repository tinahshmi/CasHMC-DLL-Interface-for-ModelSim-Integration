#include "MemoryAPI.h"

namespace CasHMC
{

MemoryAPI::MemoryAPI(
        std::ofstream &debugOut,
        std::ofstream &stateOut)
:
DualVectorObject<Packet, Packet>(
        debugOut,
        stateOut,
        MEMORY_API_BUFFER_SIZE,
        MEMORY_API_BUFFER_SIZE)
{
    std::cout << "========== CasHMC Configuration ==========" << std::endl;
    std::cout << "NUM_BANKS   = " << NUM_BANKS << std::endl;
    std::cout << "NUM_ROWS    = " << NUM_ROWS << std::endl;
    std::cout << "NUM_COLS    = " << NUM_COLS << std::endl;
    std::cout << "MAX_CMD_QUE = " << MAX_CMD_QUE << std::endl;
    std::cout << "MAX_VLT_BUF = " << MAX_VLT_BUF << std::endl;
    std::cout << "MEMORY_API_BUFFER = "<< MEMORY_API_BUFFER_SIZE<< std::endl;
    std::cout << "OPEN_PAGE   = " << OPEN_PAGE << std::endl;
    std::cout << "==========================================" << std::endl;

    vaultControllers.reserve(MEMORY_API_NUM_VAULTS);
    drams.reserve(MEMORY_API_NUM_VAULTS);

    responseQueues.resize(MEMORY_API_NUM_VAULTS);

    for (unsigned v = 0; v < MEMORY_API_NUM_VAULTS; ++v)
    {
        VaultController *vc =
            new VaultController(debugOut, stateOut, v);

        DRAM *dram =
            new DRAM(debugOut, stateOut, v, vc);

        vc->dramP = dram;
        vc->upBufferDest = this;

        vaultControllers.push_back(vc);
        drams.push_back(dram);

        std::cout << "Created VaultController "<< v<< " with DRAM "<< v<< std::endl;
    }

}

MemoryAPI::~MemoryAPI()
{
    for (unsigned v = 0; v < MEMORY_API_NUM_VAULTS; ++v)
    {
        delete vaultControllers[v];
        delete drams[v];
    }

    vaultControllers.clear();
    drams.clear();
}

void MemoryAPI::Update()
{
    //-------------------------------------------------
    // Step 1 : Send one request to VaultController
    //-------------------------------------------------
    // std::cout << "Update() begin" << std::endl;
    std::cout << "\n===== MemoryAPI Update =====" << std::endl;
    std::cout << "downBuffers.size = " << downBuffers.size() << std::endl;
    std::cout << "upBuffers.size   = " << upBuffers.size() << std::endl;
    std::cout << "Response queues:" << std::endl;
    for (unsigned v = 0; v < MEMORY_API_NUM_VAULTS; ++v)
    {
        if (!responseQueues[v].empty())
        {
            std::cout << "  Vault " << v << " : " << responseQueues[v].size() << " response(s)" << std::endl;
        }
    }
    // if (!downBuffers.empty())
    // {
    //     Packet *packet = downBuffers.front();

    //     unsigned vaultID = outstandingRequests[packet->TAG].vaultID;

    //     if (vaultID >= MEMORY_API_NUM_VAULTS)
    //     {
    //         std::cout << "Invalid vaultID = "
    //                 << vaultID
    //                 << std::endl;
    //         return;
    //     }

    //     std::cout << "Sending packet TAG "
    //             << packet->TAG
    //             << " to VaultController "
    //             << vaultID
    //             << std::endl;

    //     if (vaultControllers[vaultID]->ReceiveDown(packet))
    //     {
    //         downBuffers.erase(
    //             downBuffers.begin(),
    //             downBuffers.begin() + packet->LNG
    //         );
    //     }
    // }

    //for not pending requests
    while (!downBuffers.empty())
    {
        Packet *packet = downBuffers.front();

        unsigned vaultID = outstandingRequests[packet->TAG].vaultID;

        if (vaultID >= MEMORY_API_NUM_VAULTS)
        {
            std::cout << "Invalid vaultID = "
                    << vaultID
                    << std::endl;
            break;
        }

        std::cout << "Sending packet TAG "
                << packet->TAG
                << " to VaultController "
                << vaultID
                << std::endl;

        if (vaultControllers[vaultID]->ReceiveDown(packet))
        {
            downBuffers.erase(
                downBuffers.begin(),
                downBuffers.begin() + packet->LNG
            );
        }
        else
        {
            break;
        }
    }

    //-------------------------------------------------
    // Step 2 : Advance memory system
    //-------------------------------------------------
    for (unsigned v = 0; v < MEMORY_API_NUM_VAULTS; ++v)
    {
        vaultControllers[v]->Update();
        drams[v]->Update();
    }

    //-------------------------------------------------
    // Step 3 : Move responses out of local upBuffer
    // The packet itself is NOT deleted here.
    // CallbackReceiveUp() already stored the packet
    // pointer in responseQueues[vaultID].
    // Therefore:
    //     upBuffers      -> releases transport storage
    //     responseQueues -> keeps application response
    // The response remains available until GetResponse()
    // is explicitly called for that vault.
    //-------------------------------------------------
    
    while (!upBuffers.empty())
    {
        Packet *packet = upBuffers.front();

        if (packet == NULL)
        {
            std::cerr << "ERROR: MemoryAPI received NULL response packet"
                    << std::endl;
            exit(1);
        }

        std::cout << "MemoryAPI consuming response packet"
                << " TAG=" << packet->TAG
                << " LNG=" << packet->LNG
                << std::endl;

        unsigned packetLNG = packet->LNG;

        upBuffers.erase(
            upBuffers.begin(),
            upBuffers.begin() + packetLNG
        );
    }

    //-------------------------------------------------
    // Step 4 : Advance local clock
    //-------------------------------------------------
    Step();
}

void MemoryAPI::CallbackReceiveDown(Packet *packet, bool chkReceive)
{
     if(chkReceive)
    {
        std::cout << "\n===== VaultController ReceiveDown =====" << std::endl;

        std::cout << "TAG     : "
                  << packet->TAG
                  << std::endl;

        std::cout << "Address : 0x"
                  << std::hex
                  << packet->ADRS
                  << std::dec
                  << std::endl;

        std::cout << "Size    : "
                  << packet->reqDataSize
                  << " Bytes"
                  << std::endl;

        std::cout << "Command : "
                  << packet->CMD
                  << std::endl;

        std::cout << "======================================="
                  << std::endl;
    }
    else
    {
        std::cout << "Vault buffer FULL!" << std::endl;
    }
}

void MemoryAPI::CallbackReceiveUp(Packet *packet,
                                  bool chkReceive)
{
    std::cout << "\n===== CallbackReceiveUp =====" << std::endl;
    std::cout << "chkReceive      = " << chkReceive << std::endl;
    std::cout << "packet TAG      = " << packet->TAG << std::endl;
    std::cout << "packet LNG      = " << packet->LNG << std::endl;
    std::cout << "upBuffers.size  = " << upBuffers.size() << std::endl;
    std::cout << "upBufferMax     = " << upBufferMax << std::endl;
    std::cout << "required        = "
              << upBuffers.size() + packet->LNG
              << std::endl;
    std::cout << "=============================" << std::endl;

    if (!chkReceive)
    {
        std::cout << "Response rejected." << std::endl;
        return;
    }

    //--------------------------------------------------
    // Find original request using TAG
    //--------------------------------------------------

    auto it = outstandingRequests.find(packet->TAG);

    if (it == outstandingRequests.end())
    {
        std::cout << "ERROR: Response TAG "
                  << packet->TAG
                  << " not found in outstanding request table."
                  << std::endl;

        return;
    }

    unsigned vaultID = it->second.vaultID;

    if (vaultID >= MEMORY_API_NUM_VAULTS)
    {
        std::cout << "ERROR: Invalid vaultID = "
                  << vaultID
                  << " for response TAG "
                  << packet->TAG
                  << std::endl;

        return;
    }

    //--------------------------------------------------
    // Store response in the queue belonging to
    // this vault.
    // Do NOT delete the packet here.
    //--------------------------------------------------

    responseQueues[vaultID].push(packet);

    std::cout << "Response stored:"
              << " TAG=" << packet->TAG
              << " vaultID=" << vaultID
              << std::endl;

    std::cout << "Vault "
              << vaultID
              << " response queue size = "
              << responseQueues[vaultID].size()
              << std::endl;
}

bool MemoryAPI::Send(Packet *packet)
{
    return ReceiveDown(packet);
}

bool MemoryAPI::Reset()
{
    for (unsigned v = 0;
         v < MEMORY_API_NUM_VAULTS;
         ++v)
    {
        while (!responseQueues[v].empty())
        {
            delete responseQueues[v].front();
            responseQueues[v].pop();
        }
    }

    outstandingRequests.clear();

    downBuffers.clear();

    upBuffers.clear();

    return true;
}

bool MemoryAPI::Read(unsigned vaultID,
                     uint64_t address,
                     unsigned bytes)
{
    if (vaultID >= MEMORY_API_NUM_VAULTS)
    return false;

    PacketCommandType cmd;

    switch(bytes)
    {
        case 16:  cmd = RD16;  break;
        case 32:  cmd = RD32;  break;
        case 48:  cmd = RD48;  break;
        case 64:  cmd = RD64;  break;
        case 80:  cmd = RD80;  break;
        case 96:  cmd = RD96;  break;
        case 112: cmd = RD112; break;
        case 128: cmd = RD128; break;
        // case 256: cmd = RD256; break;

        default:
            return false;
    }

    Packet *pkt = new Packet(
            REQUEST,
            cmd,
            address,
            0,
            bytes/16 + 1,
            NULL);

    pkt->reqDataSize = bytes;

    OutstandingRequest req;

    req.address = address;
    req.bytes   = bytes;
    req.write   = false;
    req.vaultID = vaultID;

    outstandingRequests[pkt->TAG] = req;

    std::cout << "Vault downBufferMax = "
          << vaultControllers[0]->downBufferMax
          << std::endl;

    std::cout << "Vault current buffer = "
            << vaultControllers[0]->downBuffers.size()
            << std::endl;

    // std::cout << "\nSending READ packet..." << std::endl;
    // std::cout << "Address = 0x"
    //         << std::hex
    //         << address
    //         << std::dec
    //         << std::endl;

    // std::cout << "Bytes = "
    //         << bytes
    //         << std::endl;

    // std::cout << "Vault downBufferMax = "
    //         << vaultController->downBufferMax
    //         << std::endl;

    // std::cout << "Vault current buffer = "
    //         << vaultController->downBuffers.size()
    //         << std::endl;

    // std::cout << "Packet LNG = "
    //         << pkt->LNG
    //         << std::endl;

    // bool ok = vaultController->ReceiveDown(pkt);

    // std::cout << "ReceiveDown returned "
    //         << ok
    //         << std::endl;

    // return ok;
    bool ok = ReceiveDown(pkt);

    std::cout << "MemoryAPI ReceiveDown returned "
            << ok
            << std::endl;

    return ok;
}

bool MemoryAPI::Write(unsigned vaultID,
                      uint64_t address,
                      unsigned bytes)
{
    if (vaultID >= MEMORY_API_NUM_VAULTS)
    return false;

    PacketCommandType cmd;
    switch(bytes)
    {
        case 16:  cmd = WR16;  break;
        case 32:  cmd = WR32;  break;
        case 48:  cmd = WR48;  break;
        case 64:  cmd = WR64;  break;
        case 80:  cmd = WR80;  break;
        case 96:  cmd = WR96;  break;
        case 112: cmd = WR112; break;
        case 128: cmd = WR128; break;
        // case 256: cmd = WR256; break;
        default:
            return false;
    }

    Packet *pkt = new Packet(
            REQUEST,
            cmd,
            address,
            0,
            bytes/16 + 1,
            NULL);

    pkt->reqDataSize = bytes;

    OutstandingRequest req;

    req.address = address;
    req.bytes   = bytes;
    req.write   = true;
    req.vaultID = vaultID;

    outstandingRequests[pkt->TAG] = req;


    // temporary dummy payload
    pkt->DATA = new uint64_t[bytes/8];

    for(unsigned i=0;i<bytes/8;i++)
        pkt->DATA[i] = 0xAAAAAAAAAAAAAAAAULL + i;

    // std::cout << "\nSending WRITE packet..." << std::endl;
    // std::cout << "Address = 0x" << std::hex << address<< std::dec << std::endl;
    // std::cout << "Bytes = "<< bytes<< std::endl;

    bool ok = ReceiveDown(pkt);
    std::cout << "MemoryAPI ReceiveDown returned "<< ok<< std::endl;
    return ok;

    // bool ok = vaultController->ReceiveDown(pkt);

    // std::cout << "ReceiveDown returned "
    //           << ok
    //           << std::endl;

    // return ok;
}

bool MemoryAPI::HasResponse(unsigned vaultID) const
{
    if (vaultID >= MEMORY_API_NUM_VAULTS)
        return false;

    return !responseQueues[vaultID].empty();
}

bool MemoryAPI::GetResponse(unsigned vaultID,
                            MemoryResponse &rsp)
{
    if (vaultID >= MEMORY_API_NUM_VAULTS)
        return false;

    if (responseQueues[vaultID].empty())
        return false;

    //--------------------------------------------------
    // Get the oldest response belonging to this vault
    //--------------------------------------------------

    Packet *pkt = responseQueues[vaultID].front();

    responseQueues[vaultID].pop();

    if (pkt == NULL)
    {
        std::cout << "ERROR: NULL packet in response queue "
                  << "for vault " << vaultID
                  << std::endl;

        return false;
    }

    //--------------------------------------------------
    // Verify that the packet really belongs to this
    // vault using the original request information.
    //--------------------------------------------------

    auto it = outstandingRequests.find(pkt->TAG);

    if (it == outstandingRequests.end())
    {
        std::cout << "WARNING: TAG "
                  << pkt->TAG
                  << " not found in outstanding request table."
                  << std::endl;

        delete pkt;
        return false;
    }

    if (it->second.vaultID != vaultID)
    {
        std::cout << "ERROR: Vault mismatch!"
                  << " requested vault = " << vaultID
                  << " response vault = " << it->second.vaultID
                  << " TAG = " << pkt->TAG
                  << std::endl;

        // Do not silently return a wrong response.
        // Keep the packet? At this point it was popped.
        // For now, delete and report failure.
        delete pkt;
        return false;
    }

    //--------------------------------------------------
    // Fill response
    //--------------------------------------------------

    rsp.valid = true;

    rsp.tag = pkt->TAG;

    rsp.address  = it->second.address;
    rsp.bytes    = it->second.bytes;
    rsp.writeAck = it->second.write;
    rsp.vaultID  = it->second.vaultID;

    //--------------------------------------------------
    // Request is now officially completed/released.
    //--------------------------------------------------

    outstandingRequests.erase(it);

    //--------------------------------------------------
    // Now, and only now, delete the response packet.
    //--------------------------------------------------

    delete pkt;

    std::cout << "Response released:"
              << " vaultID=" << vaultID
              << " TAG=" << rsp.tag
              << std::endl;

    std::cout << "Remaining responses for vault "
              << vaultID
              << " = "
              << responseQueues[vaultID].size()
              << std::endl;

    return true;
}

}