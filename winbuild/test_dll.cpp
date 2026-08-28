#include <windows.h>
#include <iostream>
#include <cstdint>

typedef bool (__cdecl *HMC_Init_t)();
typedef void (__cdecl *HMC_Shutdown_t)();
typedef void (__cdecl *HMC_Update_t)();
typedef bool (__cdecl *HMC_Write_t)(uint64_t, unsigned);
typedef bool (__cdecl *HMC_HasResponse_t)();
typedef bool (__cdecl *HMC_GetResponse_t)(
    bool *,
    uint16_t *,
    uint64_t *,
    unsigned *);

int main()
{
    std::cout << "========================================\n";
    std::cout << "      CasHMC Windows DLL Test\n";
    std::cout << "========================================\n\n";

    std::cout << "[1] Loading cashmc.dll...\n";

    HMODULE h = LoadLibraryA("cashmc.dll");

    if (!h)
    {
        std::cout << "ERROR: LoadLibrary failed. Error = "
                  << GetLastError() << "\n";
        return 1;
    }

    std::cout << "DLL loaded successfully.\n";

    HMC_Init_t HMC_Init =
        (HMC_Init_t)GetProcAddress(h, "HMC_Init");

    HMC_Shutdown_t HMC_Shutdown =
        (HMC_Shutdown_t)GetProcAddress(h, "HMC_Shutdown");

    HMC_Update_t HMC_Update =
        (HMC_Update_t)GetProcAddress(h, "HMC_Update");

    HMC_Write_t HMC_Write =
        (HMC_Write_t)GetProcAddress(h, "HMC_Write");

    HMC_HasResponse_t HMC_HasResponse =
        (HMC_HasResponse_t)GetProcAddress(h, "HMC_HasResponse");

    HMC_GetResponse_t HMC_GetResponse =
        (HMC_GetResponse_t)GetProcAddress(h, "HMC_GetResponse");

    if (!HMC_Init ||
        !HMC_Shutdown ||
        !HMC_Update ||
        !HMC_Write ||
        !HMC_HasResponse ||
        !HMC_GetResponse)
    {
        std::cout << "ERROR: One or more functions were not found.\n";

        FreeLibrary(h);
        return 1;
    }

    std::cout << "All required HMC functions found.\n\n";


    // -------------------------------------------------
    // Initialization
    // -------------------------------------------------

    std::cout << "[2] Initializing CasHMC...\n";

    if (!HMC_Init())
    {
        std::cout << "ERROR: HMC_Init() failed.\n";

        FreeLibrary(h);
        return 1;
    }

    std::cout << "HMC_Init() SUCCESS\n\n";


    // -------------------------------------------------
    // WRITE
    // -------------------------------------------------

    uint64_t address = 0x1000;
    unsigned bytes = 64;

    std::cout << "[3] Sending WRITE...\n";
    std::cout << "Address = 0x"
              << std::hex << address << std::dec << "\n";
    std::cout << "Bytes   = " << bytes << "\n";

    if (!HMC_Write(address, bytes))
    {
        std::cout << "ERROR: HMC_Write() failed.\n";

        HMC_Shutdown();
        FreeLibrary(h);
        return 1;
    }

    std::cout << "HMC_Write() SUCCESS\n\n";


    // -------------------------------------------------
    // Run CasHMC cycles
    // -------------------------------------------------

    std::cout << "[4] Running CasHMC cycles...\n";

    bool responseFound = false;

    const unsigned MAX_CYCLES = 1000;

    unsigned responseCycle = 0;

    for (unsigned cycle = 0;
         cycle < MAX_CYCLES;
         ++cycle)
    {
        HMC_Update();

        if (HMC_HasResponse())
        {
            responseFound = true;
            responseCycle = cycle;
            break;
        }
    }


    // -------------------------------------------------
    // Response
    // -------------------------------------------------

    if (!responseFound)
    {
        std::cout << "\nERROR: No response detected after "
                  << MAX_CYCLES << " cycles.\n";

        HMC_Shutdown();
        FreeLibrary(h);
        return 1;
    }

    std::cout << "\nResponse detected at cycle "
              << responseCycle << "\n";


    bool writeAck = false;
    uint16_t tag = 0;
    uint64_t responseAddress = 0;
    unsigned responseBytes = 0;

    if (!HMC_GetResponse(
            &writeAck,
            &tag,
            &responseAddress,
            &responseBytes))
    {
        std::cout << "ERROR: HMC_GetResponse() failed.\n";

        HMC_Shutdown();
        FreeLibrary(h);
        return 1;
    }


    std::cout << "\n[5] Response received:\n";

    std::cout << "  writeAck = "
              << (writeAck ? 1 : 0) << "\n";

    std::cout << "  tag      = "
              << tag << "\n";

    std::cout << "  address  = 0x"
              << std::hex << responseAddress
              << std::dec << "\n";

    std::cout << "  bytes    = "
              << responseBytes << "\n";


    // -------------------------------------------------
    // Verification
    // -------------------------------------------------

    bool pass = true;

    if (!writeAck)
    {
        std::cout << "ERROR: writeAck is not true.\n";
        pass = false;
    }

    if (tag != 0)
    {
        std::cout << "ERROR: Unexpected tag.\n";
        pass = false;
    }

    if (responseAddress != address)
    {
        std::cout << "ERROR: Address mismatch.\n";
        pass = false;
    }

    if (responseBytes != bytes)
    {
        std::cout << "ERROR: Byte count mismatch.\n";
        pass = false;
    }


    // -------------------------------------------------
    // Shutdown
    // -------------------------------------------------

    std::cout << "\n[6] Shutting down CasHMC...\n";

    HMC_Shutdown();

    std::cout << "HMC_Shutdown() SUCCESS\n\n";

    FreeLibrary(h);


    if (pass)
    {
        std::cout << "========================================\n";
        std::cout << "             TEST PASSED\n";
        std::cout << "========================================\n";

        return 0;
    }

    std::cout << "========================================\n";
    std::cout << "             TEST FAILED\n";
    std::cout << "========================================\n";

    return 1;
}