#include <iostream>
#include <cstdint>

#include "../sources/CasHMCDLL.h"


struct TestRequest
{
    unsigned vaultID;
    bool write;
    uint64_t address;
    unsigned bytes;
    bool completed;
};


int main()
{
    std::cout << "========================================\n";
    std::cout << "   CasHMC 16-Vault DLL Interface Test\n";
    std::cout << "   Per-Vault Response Queue Version\n";
    std::cout << "========================================\n";


    // --------------------------------------------------
    // 1. Initialize CasHMC
    // --------------------------------------------------
    std::cout << "\n[1] Initializing CasHMC...\n";

    if (!HMC_Init())
    {
        std::cerr << "ERROR: HMC_Init() failed.\n";
        return 1;
    }

    std::cout << "HMC_Init() SUCCESS\n";


    // --------------------------------------------------
    // 2. Prepare 16 requests
    // --------------------------------------------------
    TestRequest requests[16];

    const unsigned sizes[8] =
    {
        16, 32, 48, 64,
        80, 96, 112, 128
    };

    std::cout << "\n[2] Preparing 16 requests...\n";

    for (unsigned v = 0; v < 16; v++)
    {
        requests[v].vaultID = v;

        // Alternate WRITE / READ
        requests[v].write = (v % 2 == 0);

        requests[v].address =
            0x1000ULL * (v + 1);

        requests[v].bytes =
            sizes[v % 8];

        requests[v].completed = false;

        std::cout << "Vault " << v
                  << " : "
                  << (requests[v].write ? "WRITE" : "READ")
                  << " | Address = 0x"
                  << std::hex << requests[v].address
                  << std::dec
                  << " | Bytes = "
                  << requests[v].bytes
                  << "\n";
    }


    // --------------------------------------------------
    // 3. Submit ALL 16 requests before Update()
    // --------------------------------------------------
    std::cout << "\n[3] Submitting all 16 requests...\n";

    unsigned accepted = 0;

    for (unsigned v = 0; v < 16; v++)
    {
        bool ok;

        if (requests[v].write)
        {
            ok = HMC_Write(
                requests[v].vaultID,
                requests[v].address,
                requests[v].bytes);
        }
        else
        {
            ok = HMC_Read(
                requests[v].vaultID,
                requests[v].address,
                requests[v].bytes);
        }

        if (ok)
        {
            accepted++;

            std::cout << "Accepted: "
                      << (requests[v].write ? "WRITE" : "READ")
                      << " | Vault = "
                      << requests[v].vaultID
                      << " | Address = 0x"
                      << std::hex << requests[v].address
                      << std::dec
                      << " | Bytes = "
                      << requests[v].bytes
                      << "\n";
        }
        else
        {
            std::cerr << "REJECTED: Vault "
                      << requests[v].vaultID
                      << "\n";
        }
    }

    std::cout << "\nAccepted requests = "
              << accepted
              << " / 16\n";

    if (accepted != 16)
    {
        std::cerr << "ERROR: Not all 16 requests were accepted.\n";
        HMC_Shutdown();
        return 1;
    }


    // --------------------------------------------------
    // 4. Run simulation
    // --------------------------------------------------
    std::cout << "\n[4] Running CasHMC cycles...\n";

    const int MAX_CYCLES = 1000;

    unsigned responsesReceived = 0;
    unsigned maxResponsesPerCycle = 0;

    for (int cycle = 0; cycle < MAX_CYCLES; cycle++)
    {
        // Advance CasHMC by one cycle.
        HMC_Update();

        unsigned responsesThisCycle = 0;


        // --------------------------------------------------
        // Check every vault independently.
        //
        // A response belongs to the queue associated with
        // the vault stored when the request was submitted.
        // --------------------------------------------------
        for (unsigned v = 0; v < 16; v++)
        {
            if (!HMC_HasResponse(v))
                continue;


            // --------------------------------------------------
            // Release one response from vault v.
            // --------------------------------------------------
            bool writeAck = false;
            uint16_t tag = 0;
            uint64_t responseAddress = 0;
            unsigned responseBytes = 0;
            unsigned responseVault = 0;

            if (!HMC_GetResponse(
                    v,
                    &writeAck,
                    &tag,
                    &responseAddress,
                    &responseBytes,
                    &responseVault))
            {
                std::cerr << "ERROR: HMC_GetResponse("
                          << v
                          << ") failed.\n";

                HMC_Shutdown();
                return 1;
            }


            responsesThisCycle++;
            responsesReceived++;


            std::cout << "Cycle "
                      << cycle
                      << " response: "
                      << "QUERY_VAULT=" << v
                      << " | RESPONSE_VAULT=" << responseVault
                      << " | TAG=" << tag
                      << " | writeAck=" << writeAck
                      << " | Address=0x"
                      << std::hex << responseAddress
                      << std::dec
                      << " | Bytes=" << responseBytes
                      << "\n";


            // --------------------------------------------------
            // Verify response vault.
            //
            // The response must come from the same vault whose
            // queue was queried.
            // --------------------------------------------------
            if (responseVault != v)
            {
                std::cerr << "ERROR: Response vault mismatch.\n";
                std::cerr << "       Queried vault    = "
                          << v
                          << "\n";
                std::cerr << "       Response vault  = "
                          << responseVault
                          << "\n";

                HMC_Shutdown();
                return 1;
            }


            // --------------------------------------------------
            // Find corresponding request.
            // --------------------------------------------------
            bool found = false;

            for (unsigned req = 0; req < 16; req++)
            {
                if (requests[req].completed)
                    continue;

                if (requests[req].vaultID != responseVault)
                    continue;

                if (requests[req].address != responseAddress)
                    continue;

                if (requests[req].bytes != responseBytes)
                    continue;

                if (requests[req].write != writeAck)
                    continue;


                requests[req].completed = true;
                found = true;


                std::cout << "  -> Matched Vault "
                          << requests[req].vaultID
                          << "\n";

                break;
            }


            if (!found)
            {
                std::cerr << "ERROR: Response could not be matched.\n";

                HMC_Shutdown();
                return 1;
            }


            // --------------------------------------------------
            // Since this test submits only one request per vault,
            // the vault queue should now be empty.
            // --------------------------------------------------
            if (HMC_HasResponse(v))
            {
                std::cerr << "ERROR: Vault "
                          << v
                          << " still has a response after GetResponse().\n";

                HMC_Shutdown();
                return 1;
            }

            std::cout << "  -> Vault "
                      << v
                      << " response queue is now empty.\n";
        }


        // --------------------------------------------------
        // Track maximum responses released in one cycle.
        // --------------------------------------------------
        if (responsesThisCycle > maxResponsesPerCycle)
        {
            maxResponsesPerCycle = responsesThisCycle;
        }


        if (responsesThisCycle > 0)
        {
            std::cout << "Cycle "
                      << cycle
                      << ": "
                      << responsesThisCycle
                      << " response(s) released\n";
        }


        // --------------------------------------------------
        // Stop once all 16 responses have been released.
        // --------------------------------------------------
        if (responsesReceived == 16)
        {
            std::cout << "\nAll 16 responses received at cycle "
                      << cycle
                      << "\n";

            break;
        }
    }


    // --------------------------------------------------
    // 5. Validate final result
    // --------------------------------------------------
    std::cout << "\n[5] Validating final result...\n";


    if (responsesReceived != 16)
    {
        std::cerr << "ERROR: Expected 16 responses, received "
                  << responsesReceived
                  << ".\n";

        HMC_Shutdown();
        return 1;
    }


    // --------------------------------------------------
    // Verify every request completed.
    // --------------------------------------------------
    for (unsigned v = 0; v < 16; v++)
    {
        if (!requests[v].completed)
        {
            std::cerr << "ERROR: Vault "
                      << v
                      << " did not complete.\n";

            HMC_Shutdown();
            return 1;
        }
    }

    std::cout << "All 16 vault requests completed.\n";


    // --------------------------------------------------
    // Verify that every vault response queue is empty.
    // --------------------------------------------------
    unsigned nonEmptyQueues = 0;

    for (unsigned v = 0; v < 16; v++)
    {
        if (HMC_HasResponse(v))
        {
            std::cerr << "ERROR: Vault "
                      << v
                      << " response queue is not empty.\n";

            nonEmptyQueues++;
        }
    }


    if (nonEmptyQueues != 0)
    {
        std::cerr << "ERROR: "
                  << nonEmptyQueues
                  << " vault response queue(s) are still non-empty.\n";

        HMC_Shutdown();
        return 1;
    }

    std::cout << "All 16 vault response queues are empty.\n";


    // --------------------------------------------------
    // 6. Print final summary
    // --------------------------------------------------
    std::cout << "\n========================================\n";
    std::cout << "          FINAL DLL TEST RESULTS\n";
    std::cout << "========================================\n";

    for (unsigned v = 0; v < 16; v++)
    {
        std::cout << "Vault "
                  << v
                  << " | "
                  << (requests[v].write ? "WRITE" : "READ")
                  << " | Address = 0x"
                  << std::hex << requests[v].address
                  << std::dec
                  << " | Bytes = "
                  << requests[v].bytes
                  << " | COMPLETED\n";
    }

    std::cout << "\n========================================\n";

    std::cout << "Requests submitted      : 16\n";
    std::cout << "Requests accepted       : "
              << accepted
              << "\n";

    std::cout << "Responses released      : "
              << responsesReceived
              << "\n";

    std::cout << "Requests completed      : 16\n";

    std::cout << "Non-empty queues        : "
              << nonEmptyQueues
              << "\n";

    std::cout << "Max responses / cycle  : "
              << maxResponsesPerCycle
              << "\n";

    std::cout << "========================================\n";

    if (responsesReceived == 16 &&
        accepted == 16 &&
        nonEmptyQueues == 0)
    {
        std::cout << "\nRESULT: PASS\n";
    }
    else
    {
        std::cout << "\nRESULT: FAIL\n";

        HMC_Shutdown();
        return 1;
    }


    // --------------------------------------------------
    // 7. Shutdown
    // --------------------------------------------------
    std::cout << "\n[6] Shutting down CasHMC...\n";

    HMC_Shutdown();

    std::cout << "HMC_Shutdown() SUCCESS\n";

    std::cout << "\n========================================\n";
    std::cout << "             DLL TEST PASSED\n";
    std::cout << "========================================\n";

    return 0;
}