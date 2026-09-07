// #include <iostream>
// #include <fstream>
// #include <cstdint>

// #include "MemoryAPI.h"
// #include "ConfigReader.h"

// using namespace CasHMC;

// int main()
// {
//     //-------------------------------------
//     // Load CasHMC configuration
//     //-------------------------------------
//     ReadIniFile("ConfigSim.ini");
//     ReadIniFile("ConfigDRAM.ini");

//     std::ofstream debugOut("debug.log");
//     std::ofstream stateOut("state.log");

//     MemoryAPI mem(debugOut, stateOut);

//     //------------------------------------------------
//     // Test requests
//     //------------------------------------------------
//     //
//     // First 16 requests:
//     //   One request for every vault.
//     //
//     // Additional requests:
//     //   Two extra requests for Vault 12.
//     //
//     // Total = 18 requests.
//     //------------------------------------------------

//     struct TestRequest
//     {
//         unsigned vaultID;
//         uint64_t address;
//         unsigned bytes;
//         bool write;
//     };

//     TestRequest requests[] =
//     {
//         {0,  0x1000,  16,  true},
//         {1,  0x2000,  32,  false},
//         {2,  0x3000,  48,  true},
//         {3,  0x4000,  64,  false},
//         {4,  0x5000,  80,  true},
//         {5,  0x6000,  96,  false},
//         {6,  0x7000, 112,  true},
//         {7,  0x8000, 128, false},

//         {8,  0x9000,  16,  true},
//         {9,  0xA000,  32,  false},
//         {10, 0xB000,  48,  true},
//         {11, 0xC000,  64,  false},
//         {12, 0xD000,  80,  true},
//         {13, 0xE000,  96,  false},
//         {14, 0xF000, 112,  true},
//         {15, 0x10000, 128, false},

//         // Two additional requests for Vault 12
//         {12, 0x11000, 16,  false},
//         {12, 0x12000, 32,  true}
//     };

//     const unsigned NUM_TEST_REQUESTS =
//         sizeof(requests) / sizeof(requests[0]);

//     //------------------------------------------------
//     // Submit all requests
//     //------------------------------------------------

//     std::cout << "\n========================================\n";
//     std::cout << "Submitting test requests\n";
//     std::cout << "========================================\n";

//     unsigned acceptedCount = 0;

//     for (unsigned i = 0; i < NUM_TEST_REQUESTS; ++i)
//     {
//         TestRequest &req = requests[i];

//         bool accepted = false;

//         if (req.write)
//         {
//             accepted = mem.Write(
//                 req.vaultID,
//                 req.address,
//                 req.bytes
//             );

//             if (accepted)
//             {
//                 ++acceptedCount;

//                 std::cout << "WRITE accepted: "
//                           << "Vault=" << req.vaultID
//                           << ", Address=0x"
//                           << std::hex << req.address
//                           << std::dec
//                           << ", Bytes=" << req.bytes
//                           << std::endl;
//             }
//             else
//             {
//                 std::cout << "WRITE rejected: "
//                           << "Vault=" << req.vaultID
//                           << ", Bytes=" << req.bytes
//                           << std::endl;
//             }
//         }
//         else
//         {
//             accepted = mem.Read(
//                 req.vaultID,
//                 req.address,
//                 req.bytes
//             );

//             if (accepted)
//             {
//                 ++acceptedCount;

//                 std::cout << "READ accepted:  "
//                           << "Vault=" << req.vaultID
//                           << ", Address=0x"
//                           << std::hex << req.address
//                           << std::dec
//                           << ", Bytes=" << req.bytes
//                           << std::endl;
//             }
//             else
//             {
//                 std::cout << "READ rejected:  "
//                           << "Vault=" << req.vaultID
//                           << ", Bytes=" << req.bytes
//                           << std::endl;
//             }
//         }
//     }

//     std::cout << "\nAccepted requests = "
//               << acceptedCount
//               << " / "
//               << NUM_TEST_REQUESTS
//               << std::endl;

//     if (acceptedCount != NUM_TEST_REQUESTS)
//     {
//         std::cout << "ERROR: Not all requests were accepted."
//                   << std::endl;
//         return 1;
//     }

//     //------------------------------------------------
//     // Test state
//     //------------------------------------------------

//     unsigned responseCount = 0;

//     bool vault8Released = false;
//     bool vault12FirstReleased = false;

//     bool vault12StayedReadyAfterVault8Release = false;

//     bool multipleVault12ResponsesObserved = false;

//     //------------------------------------------------
//     // Run simulation
//     //------------------------------------------------

//     std::cout << "\n========================================\n";
//     std::cout << "Starting simulation\n";
//     std::cout << "========================================\n";

//     for (unsigned cycle = 0;
//          cycle < 500 && responseCount < NUM_TEST_REQUESTS;
//          ++cycle)
//     {
//         std::cout << "\n========== Cycle "
//                   << cycle
//                   << " ==========" << std::endl;

//         //------------------------------------------------
//         // Advance CasHMC by one cycle
//         //------------------------------------------------

//         mem.Update();

//         //------------------------------------------------
//         // Print response status
//         //------------------------------------------------

//         std::cout << "\nResponse status:" << std::endl;

//         for (unsigned v = 0;
//              v < MEMORY_API_NUM_VAULTS;
//              ++v)
//         {
//             std::cout << "  Vault "
//                       << v
//                       << " : HasResponse = "
//                       << mem.HasResponse(v)
//                       << std::endl;
//         }

//         //------------------------------------------------
//         // IMPORTANT TEST 1:
//         //
//         // Wait until both Vault 8 and Vault 12 have
//         // responses available.
//         //
//         // Do NOT release either response before this.
//         //------------------------------------------------

//         if (!vault8Released &&
//             mem.HasResponse(8) &&
//             mem.HasResponse(12))
//         {
//             std::cout << "\n========================================\n";
//             std::cout << "Both Vault 8 and Vault 12 have responses\n";
//             std::cout << "========================================\n";

//             std::cout << "Before release:\n";

//             std::cout << "HasResponse(8)  = "
//                       << mem.HasResponse(8)
//                       << std::endl;

//             std::cout << "HasResponse(12) = "
//                       << mem.HasResponse(12)
//                       << std::endl;

//             //------------------------------------------------
//             // Release Vault 8
//             //------------------------------------------------

//             MemoryResponse rsp8;

//             if (!mem.GetResponse(8, rsp8))
//             {
//                 std::cout << "ERROR: GetResponse(8) failed."
//                           << std::endl;
//                 return 1;
//             }

//             ++responseCount;
//             vault8Released = true;

//             std::cout << "\n******** RELEASED VAULT 8 ********\n";

//             std::cout << "TAG       = "
//                       << rsp8.tag
//                       << std::endl;

//             std::cout << "Address   = 0x"
//                       << std::hex
//                       << rsp8.address
//                       << std::dec
//                       << std::endl;

//             std::cout << "Bytes     = "
//                       << rsp8.bytes
//                       << std::endl;

//             std::cout << "Write Ack = "
//                       << rsp8.writeAck
//                       << std::endl;

//             std::cout << "Vault ID  = "
//                       << rsp8.vaultID
//                       << std::endl;

//             std::cout << "**********************************\n";

//             //------------------------------------------------
//             // Verify response belongs to Vault 8
//             //------------------------------------------------

//             if (rsp8.vaultID != 8)
//             {
//                 std::cout << "ERROR: Returned response does not "
//                           << "belong to Vault 8."
//                           << std::endl;
//                 return 1;
//             }

//             //------------------------------------------------
//             // Verify Vault 8 is now empty
//             //------------------------------------------------

//             if (mem.HasResponse(8))
//             {
//                 std::cout << "ERROR: HasResponse(8) is still 1 "
//                           << "after releasing Vault 8."
//                           << std::endl;
//                 return 1;
//             }

//             std::cout << "PASS: HasResponse(8) became 0."
//                       << std::endl;

//             //------------------------------------------------
//             // Verify Vault 12 was NOT affected
//             //------------------------------------------------

//             if (!mem.HasResponse(12))
//             {
//                 std::cout << "ERROR: Vault 12 response disappeared "
//                           << "when Vault 8 was released."
//                           << std::endl;
//                 return 1;
//             }

//             vault12StayedReadyAfterVault8Release = true;

//             std::cout << "PASS: HasResponse(12) remains 1 "
//                       << "after releasing Vault 8."
//                       << std::endl;
//         }

//         //------------------------------------------------
//         // IMPORTANT TEST 2:
//         //
//         // Release one response from Vault 12.
//         //
//         // If another response is already queued for Vault
//         // 12, HasResponse(12) must remain 1.
//         //------------------------------------------------

//         if (vault8Released &&
//             !vault12FirstReleased &&
//             mem.HasResponse(12))
//         {
//             MemoryResponse rsp12;

//             if (!mem.GetResponse(12, rsp12))
//             {
//                 std::cout << "ERROR: GetResponse(12) failed."
//                           << std::endl;
//                 return 1;
//             }

//             ++responseCount;
//             vault12FirstReleased = true;

//             std::cout << "\n******** RELEASED VAULT 12 RESPONSE ********\n";

//             std::cout << "TAG       = "
//                       << rsp12.tag
//                       << std::endl;

//             std::cout << "Address   = 0x"
//                       << std::hex
//                       << rsp12.address
//                       << std::dec
//                       << std::endl;

//             std::cout << "Bytes     = "
//                       << rsp12.bytes
//                       << std::endl;

//             std::cout << "Write Ack = "
//                       << rsp12.writeAck
//                       << std::endl;

//             std::cout << "Vault ID  = "
//                       << rsp12.vaultID
//                       << std::endl;

//             std::cout << "*********************************************\n";

//             //------------------------------------------------
//             // Verify returned response belongs to Vault 12
//             //------------------------------------------------

//             if (rsp12.vaultID != 12)
//             {
//                 std::cout << "ERROR: Returned response does not "
//                           << "belong to Vault 12."
//                           << std::endl;
//                 return 1;
//             }

//             //------------------------------------------------
//             // If another Vault 12 response was already
//             // buffered, this proves that the per-vault
//             // response queue supports multiple responses.
//             //------------------------------------------------

//             if (mem.HasResponse(12))
//             {
//                 multipleVault12ResponsesObserved = true;

//                 std::cout << "\nPASS: Vault 12 still has a "
//                           << "pending response after releasing "
//                           << "one response."
//                           << std::endl;
//             }
//             else
//             {
//                 std::cout << "\nVault 12 has no additional "
//                           << "response buffered yet."
//                           << std::endl;

//                 std::cout << "The simulation will continue so "
//                           << "remaining Vault 12 requests can "
//                           << "complete."
//                           << std::endl;
//             }
//         }

//         //------------------------------------------------
//         // Release all responses that are currently ready.
//         //
//         // This is deliberately done by vault ID.
//         //
//         // We do NOT use a global response queue.
//         //------------------------------------------------

//         if (vault8Released && vault12FirstReleased)
//         {
//             for (unsigned v = 0;
//                  v < MEMORY_API_NUM_VAULTS;
//                  ++v)
//             {
//                 //------------------------------------------------
//                 // Do not release Vault 12 here if we have not
//                 // yet checked whether multiple responses can
//                 // remain queued.
//                 //
//                 // After the first Vault 12 response, however,
//                 // every later ready response can be released.
//                 //------------------------------------------------

//                 while (mem.HasResponse(v))
//                 {
//                     MemoryResponse rsp;

//                     if (!mem.GetResponse(v, rsp))
//                     {
//                         std::cout
//                             << "ERROR: GetResponse failed for "
//                             << "Vault "
//                             << v
//                             << std::endl;

//                         return 1;
//                     }

//                     //------------------------------------------------
//                     // Verify vault mapping.
//                     //------------------------------------------------

//                     if (rsp.vaultID != v)
//                     {
//                         std::cout
//                             << "ERROR: Vault mismatch!"
//                             << " Requested vault = "
//                             << v
//                             << ", Response vault = "
//                             << rsp.vaultID
//                             << std::endl;

//                         return 1;
//                     }

//                     ++responseCount;

//                     std::cout
//                         << "Released response:"
//                         << " Vault=" << rsp.vaultID
//                         << " TAG=" << rsp.tag
//                         << std::endl;
//                 }
//             }
//         }

//         //------------------------------------------------
//         // Stop only when ALL expected responses have
//         // actually been released.
//         //------------------------------------------------

//         if (responseCount == NUM_TEST_REQUESTS)
//         {
//             std::cout << "\nAll expected responses have "
//                       << "been released."
//                       << std::endl;

//             break;
//         }
//     }

//     //------------------------------------------------
//     // Final verification
//     //------------------------------------------------

//     bool allQueuesEmpty = true;

//     std::cout << "\n========================================\n";
//     std::cout << "Final Vault Response Status\n";
//     std::cout << "========================================\n";

//     for (unsigned v = 0;
//          v < MEMORY_API_NUM_VAULTS;
//          ++v)
//     {
//         bool ready = mem.HasResponse(v);

//         std::cout << "Vault "
//                   << v
//                   << " : HasResponse = "
//                   << ready
//                   << std::endl;

//         if (ready)
//         {
//             allQueuesEmpty = false;
//         }
//     }

//     //------------------------------------------------
//     // Test summary
//     //------------------------------------------------

//     std::cout << "\n========================================\n";
//     std::cout << "Test Summary\n";
//     std::cout << "========================================\n";

//     std::cout << "Requests submitted : "
//               << NUM_TEST_REQUESTS
//               << std::endl;

//     std::cout << "Requests accepted  : "
//               << acceptedCount
//               << std::endl;

//     std::cout << "Responses released : "
//               << responseCount
//               << std::endl;

//     std::cout << "Expected responses : "
//               << NUM_TEST_REQUESTS
//               << std::endl;

//     std::cout << "Vault 8 released   : "
//               << vault8Released
//               << std::endl;

//     std::cout << "Vault 12 released  : "
//               << vault12FirstReleased
//               << std::endl;

//     std::cout << "Vault 12 survived "
//               << "Vault 8 release : "
//               << vault12StayedReadyAfterVault8Release
//               << std::endl;

//     std::cout << "Multiple Vault 12 responses observed : "
//               << multipleVault12ResponsesObserved
//               << std::endl;

//     std::cout << "All response queues empty : "
//               << allQueuesEmpty
//               << std::endl;

//     //------------------------------------------------
//     // Final PASS / FAIL
//     //------------------------------------------------

//     if (acceptedCount == NUM_TEST_REQUESTS &&
//         responseCount == NUM_TEST_REQUESTS &&
//         vault8Released &&
//         vault12FirstReleased &&
//         vault12StayedReadyAfterVault8Release &&
//         allQueuesEmpty)
//     {
//         std::cout << "\nRESULT: PASS\n";
//     }
//     else
//     {
//         std::cout << "\nRESULT: FAIL\n";
//     }

//     std::cout << "========================================\n";

//     return 0;
// }
