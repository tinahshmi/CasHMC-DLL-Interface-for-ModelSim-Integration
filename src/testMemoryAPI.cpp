
#include <iostream>
#include <fstream>

#include "MemoryAPI.h"
#include "ConfigReader.h"

using namespace CasHMC;

int main()
{
    //-------------------------------------
    // Load CasHMC configuration
    //-------------------------------------
    ReadIniFile("ConfigSim.ini");
    ReadIniFile("ConfigDRAM.ini");

    std::ofstream debugOut("debug.log");
    std::ofstream stateOut("state.log");

    MemoryAPI mem(debugOut, stateOut);
    //------------------------------------------------
    // Read
    //------------------------------------------------
    //std::cout << "Sending one read request..." << std::endl;
    //test step 1
    // if(mem.Read(0x1000, 64))
    // if(mem.Read(0x1000, 16))
    // if(mem.Read(0x1000, 32))
    // if(mem.Read(0x1000, 48))
    //     std::cout << "Read accepted." << std::endl;
    // else
    //     std::cout << "Read rejected." << std::endl;

    //test step 2  with multiple read request
    // if(mem.Read(0x1000, 48))
    // std::cout << "Read #1 accepted." << std::endl;
    // else
    //     std::cout << "Read #1 rejected." << std::endl;

    // std::cout << "Sending second read..." << std::endl;

    // if(mem.Read(0x2000, 48))
    //     std::cout << "Read #2 accepted." << std::endl;
    // else
    //     std::cout << "Read #2 rejected." << std::endl;

    //test 4 request(2 of them go to same bank)
    // if(mem.Read(0x3000, 48))
    //     std::cout << "Read #1 accepted." << std::endl;
    // else
    //     std::cout << "Read #1 rejected." << std::endl;
    // if(mem.Read(0x4000, 48))
    //     std::cout << "Read #2 accepted." << std::endl;
    // else
    //     std::cout << "Read #1 rejected." << std::endl;
    // if(mem.Read(0x5000, 48))
    //     std::cout << "Read #3 accepted." << std::endl;
    // else
    //     std::cout << "Read #1 rejected." << std::endl;

    //------------------------------------------------
    // Write
    //------------------------------------------------
    // if(mem.Write(0x1000,64))
    //     std::cout << "Write #1 accepted." << std::endl;
    // else
    //     std::cout << "Write #1 rejected." << std::endl;
    // if(mem.Write(0x2000,32))
    //     std::cout << "Write #2 accepted." << std::endl;
    // else
    //     std::cout << "Write #2 rejected." << std::endl;
    // if(mem.Write(0x3000,16))
    //     std::cout << "Write #3 accepted." << std::endl;
    // else
    //     std::cout << "Write #3 rejected." << std::endl;
    // if(mem.Write(0x3000,48))
    //     std::cout << "Write #4 accepted." << std::endl;
    // else
    //     std::cout << "Write #4 rejected." << std::endl;

        
    //------------------------------------------------
    // Read AND Write
    //------------------------------------------------
    if(mem.Write(0x1000,64))
        std::cout << "Write #1 accepted." << std::endl;
    else
        std::cout << "Write #1 rejected." << std::endl;
    
    if(mem.Read(0x1000, 64))
    std::cout << "Read #1 accepted." << std::endl;
    else
        std::cout << "Read #1 rejected." << std::endl;
    //------------------------------------------------
    // Run the memory for some cycles
    //------------------------------------------------

    for(int i = 0; i < 100; i++)
    {   
        //v1
        // std::cout << "Cycle " << i << std::endl;
        // mem.Update();
            std::cout << "Cycle " << i << std::endl;

    mem.Update();
    
    MemoryResponse rsp;

    if(mem.GetResponse(rsp))
    {
        std::cout << "\n******** APPLICATION RECEIVED RESPONSE ********\n";

        std::cout << "TAG = "
                << rsp.tag
                << std::endl;

        std::cout << "Address = 0x"
                << std::hex
                << rsp.address
                << std::dec
                << std::endl;

        std::cout << "Bytes = "
                << rsp.bytes
                << std::endl;

        std::cout << "Write Ack = "
                << rsp.writeAck
                << std::endl;

        std::cout << "***************************************"
                << std::endl;
    }
    }

    std::cout << "Finished." << std::endl;

    return 0;
}



//*********************************small DLL test program.**************************************************************
/*
#include "CasHMCDLL.h"

#include <iostream>

int main()
{
    HMC_Init();

    HMC_Write(0x1000,64);

    HMC_Read(0x1000,64);

    for(int i=0;i<100;i++)
    {
        HMC_Update();
        if(HMC_HasResponse())
        {
            bool writeAck;
            uint16_t tag;
            uint64_t addr;
            unsigned bytes;

            HMC_GetResponse(
                    &writeAck,
                    &tag,
                    &addr,
                    &bytes);
            
        std::cout << "Response " << tag << std::endl;
        std::cout << "Address = 0x" << std::hex << addr << std::dec << std::endl;
        std::cout << "Bytes = "<< bytes << std::endl;
        std::cout << "WriteAck = "<< writeAck << std::endl;
        }
    }

    HMC_Shutdown();
}

*/