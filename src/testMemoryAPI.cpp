
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
    // Test: All 16 vaults with different request sizes
    //------------------------------------------------
    //
    // Vault 0  : WRITE 16 B
    // Vault 1  : READ  32 B
    // Vault 2  : WRITE 48 B
    // Vault 3  : READ  64 B
    // Vault 4  : WRITE 80 B
    // Vault 5  : READ  96 B
    // Vault 6  : WRITE 112 B
    // Vault 7  : READ  128 B
    //
    // Vault 8  : WRITE 16 B
    // Vault 9  : READ  32 B
    // Vault 10 : WRITE 48 B
    // Vault 11 : READ  64 B
    // Vault 12 : WRITE 80 B
    // Vault 13 : READ  96 B
    // Vault 14 : WRITE 112 B
    // Vault 15 : READ  128 B
    //------------------------------------------------

    struct TestRequest
    {
        unsigned vaultID;
        uint64_t address;
        unsigned bytes;
        bool write;
    };

    TestRequest requests[] =
    {
        {0,  0x1000,  16,  true},
        {1,  0x2000,  32,  false},
        {2,  0x3000,  48,  true},
        {3,  0x4000,  64,  false},
        {4,  0x5000,  80,  true},
        {5,  0x6000,  96,  false},
        {6,  0x7000, 112,  true},
        {7,  0x8000, 128,  false},

        {8,  0x9000,  16,  true},
        {9,  0xA000,  32,  false},
        {10, 0xB000,  48,  true},
        {11, 0xC000,  64,  false},
        {12, 0xD000,  80,  true},
        {13, 0xE000,  96,  false},
        {14, 0xF000, 112,  true},
        {15, 0x10000, 128, false}
    };

    const unsigned NUM_TEST_REQUESTS =
        sizeof(requests) / sizeof(requests[0]);

    //------------------------------------------------
    // Submit all requests before Cycle 0
    //------------------------------------------------

    std::cout << "\n========================================\n";
    std::cout << "Submitting test requests\n";
    std::cout << "========================================\n";

    unsigned acceptedCount = 0;

    for (unsigned i = 0; i < NUM_TEST_REQUESTS; ++i)
    {
        TestRequest &req = requests[i];

        bool accepted;

        if (req.write)
        {
            accepted = mem.Write(
                req.vaultID,
                req.address,
                req.bytes
            );

            if (accepted)
            {
                ++acceptedCount;

                std::cout << "WRITE accepted: "
                          << "Vault=" << req.vaultID
                          << ", Address=0x"
                          << std::hex << req.address
                          << std::dec
                          << ", Bytes=" << req.bytes
                          << std::endl;
            }
            else
            {
                std::cout << "WRITE rejected: "
                          << "Vault=" << req.vaultID
                          << ", Bytes=" << req.bytes
                          << std::endl;
            }
        }
        else
        {
            accepted = mem.Read(
                req.vaultID,
                req.address,
                req.bytes
            );

            if (accepted)
            {
                ++acceptedCount;

                std::cout << "READ accepted:  "
                          << "Vault=" << req.vaultID
                          << ", Address=0x"
                          << std::hex << req.address
                          << std::dec
                          << ", Bytes=" << req.bytes
                          << std::endl;
            }
            else
            {
                std::cout << "READ rejected:  "
                          << "Vault=" << req.vaultID
                          << ", Bytes=" << req.bytes
                          << std::endl;
            }
        }
    }

    std::cout << "\nAccepted requests = "
              << acceptedCount
              << " / "
              << NUM_TEST_REQUESTS
              << std::endl;

    //------------------------------------------------
    // Run the memory for some cycles
    //------------------------------------------------

    std::cout << "\n========================================\n";
    std::cout << "Starting simulation\n";
    std::cout << "========================================\n";

    unsigned responseCount = 0;

    for (int i = 0; i < 100; i++)
    {
        std::cout << "\n========== Cycle " << i
                  << " ==========" << std::endl;

        mem.Update();

        //------------------------------------------------
        // Receive ALL responses generated in this cycle
        //------------------------------------------------

        MemoryResponse rsp;

        while (mem.GetResponse(rsp))
        {
            ++responseCount;

            std::cout << "\n******** APPLICATION RECEIVED RESPONSE ********\n";
            std::cout << "TAG       = " << rsp.tag << std::endl;
            std::cout << "Address   = 0x"
                      << std::hex << rsp.address
                      << std::dec << std::endl;
            std::cout << "Bytes     = " << rsp.bytes << std::endl;
            std::cout << "Write Ack = " << rsp.writeAck << std::endl;
            std::cout << "************************************************\n";
        }
    }

    //------------------------------------------------
    // Final test result
    //------------------------------------------------

    std::cout << "\n========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n";

    std::cout << "Requests submitted : "
              << NUM_TEST_REQUESTS
              << std::endl;

    std::cout << "Requests accepted  : "
              << acceptedCount
              << std::endl;

    std::cout << "Responses received : "
              << responseCount
              << std::endl;

    if (acceptedCount == NUM_TEST_REQUESTS &&
        responseCount == NUM_TEST_REQUESTS)
    {
        std::cout << "RESULT: PASS\n";
    }
    else
    {
        std::cout << "RESULT: FAIL\n";
    }

    std::cout << "========================================\n";

    return 0;
}