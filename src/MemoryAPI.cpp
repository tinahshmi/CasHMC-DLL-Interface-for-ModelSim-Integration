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
        MAX_VLT_BUF,
        MAX_VLT_BUF)
{
    std::cout << "========== CasHMC Configuration ==========" << std::endl;
    std::cout << "NUM_BANKS   = " << NUM_BANKS << std::endl;
    std::cout << "NUM_ROWS    = " << NUM_ROWS << std::endl;
    std::cout << "NUM_COLS    = " << NUM_COLS << std::endl;
    std::cout << "MAX_CMD_QUE = " << MAX_CMD_QUE << std::endl;
    std::cout << "MAX_VLT_BUF = " << MAX_VLT_BUF << std::endl;
    std::cout << "OPEN_PAGE   = " << OPEN_PAGE << std::endl;
    std::cout << "==========================================" << std::endl;
    vaultController = new VaultController(
            debugOut,
            stateOut,
            0);

    dram = new DRAM(
            debugOut,
            stateOut,
            0,
            vaultController);

    vaultController->dramP = dram;
    vaultController->upBufferDest = this;

}

MemoryAPI::~MemoryAPI()
{
    delete vaultController;

    delete dram;
}

void MemoryAPI::Update()
{
    //-------------------------------------------------
    // Step 1 : Send one request to VaultController
    //-------------------------------------------------
    // std::cout << "Update() begin" << std::endl;
    if (!downBuffers.empty())
    {
         std::cout << "Sending packet to VaultController" << std::endl;
        Packet *packet = downBuffers.front();

        if (vaultController->ReceiveDown(packet))
        {
            downBuffers.erase(
                downBuffers.begin(),
                downBuffers.begin() + packet->LNG
            );
        }
    }

    //-------------------------------------------------
    // Step 2 : Advance memory system
    //-------------------------------------------------
    // std::cout << "Updating VaultController" << std::endl;
    vaultController->Update();
    // std::cout << "Updating DRAM" << std::endl;
    dram->Update();;

    //-------------------------------------------------
    // Step 3 : Advance local clock
    //-------------------------------------------------
    // std::cout << "Step()" << std::endl;
    Step();
    // std::cout << "Update() end" << std::endl;
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

void MemoryAPI::CallbackReceiveUp(Packet *packet, bool chkReceive)
{
    if(!chkReceive)
    {
        std::cout << "Response rejected." << std::endl;
        return;
    }

    std::cout << "\n===== MemoryAPI received RESPONSE =====" << std::endl;

    std::cout << "TAG     : "
              << packet->TAG
              << std::endl;

    std::cout << "CMD     : "
              << packet->CMD
              << std::endl;

    std::cout << "Address : 0x"
              << std::hex
              << packet->ADRS
              << std::dec
              << std::endl;

    std::cout << "Length  : "
              << packet->LNG
              << std::endl;

    std::cout << "======================================="
              << std::endl;

    responseQueue.push(packet);
}

bool MemoryAPI::Send(Packet *packet)
{
    return ReceiveDown(packet);
}

bool MemoryAPI::Reset()
{
    while(!responseQueue.empty())
    {
        delete responseQueue.front();
        responseQueue.pop();
    }

    outstandingRequests.clear();

    downBuffers.clear();

    upBuffers.clear();

    return true;
}

bool MemoryAPI::Read(uint64_t address,
                     unsigned bytes)
{
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
        case 256: cmd = RD256; break;

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

    outstandingRequests[pkt->TAG] = req;

    std::cout << "\nSending READ packet..." << std::endl;
    std::cout << "Address = 0x"
            << std::hex
            << address
            << std::dec
            << std::endl;

    std::cout << "Bytes = "
            << bytes
            << std::endl;

    std::cout << "Vault downBufferMax = "
            << vaultController->downBufferMax
            << std::endl;

    std::cout << "Vault current buffer = "
            << vaultController->downBuffers.size()
            << std::endl;

    std::cout << "Packet LNG = "
            << pkt->LNG
            << std::endl;

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


bool MemoryAPI::Write(uint64_t address, unsigned bytes)
{
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
        case 256: cmd = WR256; break;
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

    outstandingRequests[pkt->TAG] = req;


    // temporary dummy payload
    pkt->DATA = new uint64_t[bytes/8];

    for(unsigned i=0;i<bytes/8;i++)
        pkt->DATA[i] = 0xAAAAAAAAAAAAAAAAULL + i;

    std::cout << "\nSending WRITE packet..." << std::endl;
    std::cout << "Address = 0x" << std::hex << address<< std::dec << std::endl;
    std::cout << "Bytes = "<< bytes<< std::endl;

    bool ok = ReceiveDown(pkt);
    std::cout << "MemoryAPI ReceiveDown returned "<< ok<< std::endl;
    return ok;

    // bool ok = vaultController->ReceiveDown(pkt);

    // std::cout << "ReceiveDown returned "
    //           << ok
    //           << std::endl;

    // return ok;
}

bool MemoryAPI::HasResponse() const
{
    return !responseQueue.empty();
}

bool MemoryAPI::GetResponse(MemoryResponse &rsp)
{
    if(responseQueue.empty())
        return false;

    Packet *pkt = responseQueue.front();
    responseQueue.pop();

    rsp.valid = true; 

    rsp.tag = pkt->TAG;

    //--------------------------------------------------
    // Recover original request information
    //--------------------------------------------------
    auto it = outstandingRequests.find(pkt->TAG);

    if(it != outstandingRequests.end())
    {
        rsp.address  = it->second.address;
        rsp.bytes    = it->second.bytes;
        rsp.writeAck = it->second.write;

        // This request is finished
        outstandingRequests.erase(it);
    }
    else
    {
        std::cout << "WARNING: TAG "
                  << pkt->TAG
                  << " not found in outstanding request table."
                  << std::endl;

        // Fallback values
        rsp.address  = pkt->ADRS;
        rsp.bytes    = pkt->reqDataSize;
        rsp.writeAck = (pkt->CMD == WR_RS);
    }

    //--------------------------------------------------
    // Payload
    //--------------------------------------------------
    // rsp.data = pkt->DATA;  // currently nullptr

    // No payload yet.
    // Behaviour matches original CasHMC: only completion notification.
    delete pkt;

    return true;
}

}