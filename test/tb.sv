
module tb;

    // ============================================================
    // CasHMC DPI interface
    // ============================================================

    import "DPI-C" function bit HMC_Init();
    import "DPI-C" function void HMC_Shutdown();
    import "DPI-C" function void HMC_Update();
    import "DPI-C" function void HMC_Reset();

    import "DPI-C" function bit HMC_Read(
        int unsigned vaultID,
        longint unsigned address,
        int unsigned bytes
    );

    import "DPI-C" function bit HMC_Write(
        int unsigned vaultID,
        longint unsigned address,
        int unsigned bytes
    );

    import "DPI-C" function bit HMC_HasResponse();

    import "DPI-C" function bit HMC_GetResponse(
        output bit writeAck,
        output int unsigned tag,
        output longint unsigned address,
        output int unsigned bytes
    );


    // ============================================================
    // Test parameters
    // ============================================================

    localparam int NUM_VAULTS = 16;

    int unsigned vaultID;

    longint unsigned testAddress [NUM_VAULTS];
    int unsigned     testBytes   [NUM_VAULTS];
    bit               testWrite   [NUM_VAULTS];

    bit init_ok;

    int accepted_requests;
    int received_responses;

    int cycle;

    bit writeAck;
    int unsigned tag;
    longint unsigned responseAddress;
    int unsigned responseBytes;


    // ============================================================
    // Initialize test requests
    // ============================================================

    task automatic prepare_test_requests();

        for (int v = 0; v < NUM_VAULTS; v++) begin

            vaultID = v;

            // ----------------------------------------------------
            // Different request sizes
            // ----------------------------------------------------

            case (v % 8)

                0: testBytes[v] = 16;
                1: testBytes[v] = 32;
                2: testBytes[v] = 48;
                3: testBytes[v] = 64;
                4: testBytes[v] = 80;
                5: testBytes[v] = 96;
                6: testBytes[v] = 112;
                7: testBytes[v] = 128;

            endcase


            // ----------------------------------------------------
            // Alternate WRITE / READ
            // ----------------------------------------------------

            testWrite[v] = (v % 2 == 0);


            // ----------------------------------------------------
            // Give each vault a different address
            // ----------------------------------------------------

            testAddress[v] = 64'h1000 + (v * 64'h1000);

        end

    endtask


    // ============================================================
    // Submit all 16 requests
    // ============================================================

    task automatic submit_all_requests();

        accepted_requests = 0;

        $display("");
        $display("==============================================");
        $display("       SUBMITTING 16 VAULT REQUESTS");
        $display("==============================================");

        for (int v = 0; v < NUM_VAULTS; v++) begin

            if (testWrite[v]) begin

                $display("");
                $display(
                    "[WRITE] Vault=%0d Address=0x%0h Bytes=%0d",
                    v,
                    testAddress[v],
                    testBytes[v]
                );

                if (HMC_Write(
                        v,
                        testAddress[v],
                        testBytes[v])) begin

                    accepted_requests++;

                    $display(
                        "        HMC_Write() SUCCESS"
                    );

                end
                else begin

                    $display(
                        "ERROR: HMC_Write() FAILED for Vault %0d",
                        v
                    );

                end

            end
            else begin

                $display("");
                $display(
                    "[READ ] Vault=%0d Address=0x%0h Bytes=%0d",
                    v,
                    testAddress[v],
                    testBytes[v]
                );

                if (HMC_Read(
                        v,
                        testAddress[v],
                        testBytes[v])) begin

                    accepted_requests++;

                    $display(
                        "        HMC_Read() SUCCESS"
                    );

                end
                else begin

                    $display(
                        "ERROR: HMC_Read() FAILED for Vault %0d",
                        v
                    );

                end

            end

        end


        $display("");
        $display("----------------------------------------------");
        $display("Accepted requests = %0d / %0d",
                 accepted_requests,
                 NUM_VAULTS);
        $display("----------------------------------------------");


        if (accepted_requests != NUM_VAULTS) begin

            $display("");
            $display(
                "ERROR: Not all requests were accepted!"
            );

            HMC_Shutdown();
            $fatal(1);

        end

    endtask


    // ============================================================
    // Verify one response
    //
    // The response itself does not contain vaultID.
    // Therefore we identify the expected request using
    // the TAG returned by HMC_GetResponse().
    //
    // In the current MemoryAPI implementation, the TAG is
    // associated with the original request in C++.
    // ============================================================

    task automatic verify_response();

        bit found;
        int responseVault;

        found = 0;
        responseVault = -1;


        // --------------------------------------------------------
        // Search for the request matching address + size
        // --------------------------------------------------------

        for (int v = 0; v < NUM_VAULTS; v++) begin

            if ((testAddress[v] == responseAddress) &&
                (testBytes[v]   == responseBytes)) begin

                responseVault = v;
                found = 1;
                break;

            end

        end


        // --------------------------------------------------------
        // Check that the response corresponds to a known request
        // --------------------------------------------------------

        if (!found) begin

            $display("");
            $display("ERROR: Unknown response!");
            $display("TAG     = %0d", tag);
            $display("Address = 0x%0h", responseAddress);
            $display("Bytes   = %0d", responseBytes);

            HMC_Shutdown();
            $fatal(1);

        end


        // --------------------------------------------------------
        // Verify WRITE / READ type
        // --------------------------------------------------------

        if (writeAck !== testWrite[responseVault]) begin

            $display("");
            $display("ERROR: Wrong response type!");
            $display("Vault    = %0d", responseVault);
            $display("TAG      = %0d", tag);

            $display(
                "Expected writeAck = %0d",
                testWrite[responseVault]
            );

            $display(
                "Received writeAck = %0d",
                writeAck
            );

            HMC_Shutdown();
            $fatal(1);

        end


        // --------------------------------------------------------
        // Verify address
        // --------------------------------------------------------

        if (responseAddress !== testAddress[responseVault]) begin

            $display("");
            $display("ERROR: Wrong response address!");
            $display("Vault    = %0d", responseVault);
            $display("TAG      = %0d", tag);

            $display(
                "Expected = 0x%0h",
                testAddress[responseVault]
            );

            $display(
                "Received = 0x%0h",
                responseAddress
            );

            HMC_Shutdown();
            $fatal(1);

        end


        // --------------------------------------------------------
        // Verify size
        // --------------------------------------------------------

        if (responseBytes !== testBytes[responseVault]) begin

            $display("");
            $display("ERROR: Wrong response size!");
            $display("Vault    = %0d", responseVault);
            $display("TAG      = %0d", tag);

            $display(
                "Expected = %0d",
                testBytes[responseVault]
            );

            $display(
                "Received = %0d",
                responseBytes
            );

            HMC_Shutdown();
            $fatal(1);

        end


        // --------------------------------------------------------
        // Response verified
        // --------------------------------------------------------

        $display("");
        $display(
            ">>> RESPONSE VERIFIED: Vault=%0d TAG=%0d",
            responseVault,
            tag
        );

        $display(
            "    Type    = %s",
            testWrite[responseVault] ? "WRITE ACK" : "READ"
        );

        $display(
            "    Address = 0x%0h",
            responseAddress
        );

        $display(
            "    Bytes   = %0d",
            responseBytes
        );

    endtask


    // ============================================================
    // Main test
    // ============================================================

    initial begin

        $display("");
        $display("==============================================");
        $display("       CasHMC DPI 16-VAULT TEST");
        $display("==============================================");


        // ========================================================
        // 1. Prepare requests
        // ========================================================

        prepare_test_requests();


        // ========================================================
        // 2. Initialize CasHMC
        // ========================================================

        $display("");
        $display("[1] HMC_Init()");

        init_ok = HMC_Init();

        if (!init_ok) begin

            $display("");
            $display("ERROR: HMC_Init() FAILED");

            $fatal(1);

        end

        $display("[2] HMC_Init() SUCCESS");


        // ========================================================
        // 3. Submit all 16 requests BEFORE first HMC_Update()
        // ========================================================

        submit_all_requests();


        // ========================================================
        // 4. Simulation loop
        // ========================================================

        received_responses = 0;

        $display("");
        $display("==============================================");
        $display("       STARTING CasHMC SIMULATION");
        $display("==============================================");


        for (cycle = 0; cycle < 100; cycle++) begin

            $display("");
            $display(
                "========== ModelSim Cycle %0d ==========",
                cycle
            );


            // ----------------------------------------------------
            // Advance all 16 vaults
            // ----------------------------------------------------

            HMC_Update();


            // ----------------------------------------------------
            // Consume ALL responses generated in this cycle
            // ----------------------------------------------------

            while (HMC_HasResponse()) begin

                writeAck       = 0;
                tag            = 0;
                responseAddress = 0;
                responseBytes  = 0;


                if (!HMC_GetResponse(
                        writeAck,
                        tag,
                        responseAddress,
                        responseBytes)) begin

                    $display("");
                    $display(
                        "ERROR: HMC_GetResponse() failed!"
                    );

                    HMC_Shutdown();
                    $fatal(1);

                end


                $display("");
                $display(
                    ">>> RESPONSE DETECTED at cycle %0d",
                    cycle
                );

                $display(
                    "    writeAck = %0d",
                    writeAck
                );

                $display(
                    "    tag      = %0d",
                    tag
                );

                $display(
                    "    address  = 0x%0h",
                    responseAddress
                );

                $display(
                    "    bytes    = %0d",
                    responseBytes
                );


                // ------------------------------------------------
                // Verify response
                // ------------------------------------------------

                verify_response();


                received_responses++;

            end


            // ----------------------------------------------------
            // Stop when all 16 responses have arrived
            // ----------------------------------------------------

            if (received_responses == NUM_VAULTS) begin

                $display("");
                $display(
                    "All %0d responses received at cycle %0d",
                    NUM_VAULTS,
                    cycle
                );

                break;

            end

        end


        // ========================================================
        // 5. Final result
        // ========================================================

        $display("");
        $display("==============================================");
        $display("             FINAL RESULT");
        $display("==============================================");

        $display("");
        $display(
            "Requests submitted  : %0d",
            NUM_VAULTS
        );

        $display(
            "Requests accepted   : %0d",
            accepted_requests
        );

        $display(
            "Responses received  : %0d",
            received_responses
        );


        if (received_responses != NUM_VAULTS) begin

            $display("");
            $display(
                "ERROR: Expected %0d responses but received %0d!",
                NUM_VAULTS,
                received_responses
            );

            HMC_Shutdown();
            $fatal(1);

        end


        // ========================================================
        // 6. Shutdown
        // ========================================================

        HMC_Shutdown();

        $display("");
        $display("[3] HMC_Shutdown() SUCCESS");


        $display("");
        $display("==============================================");
        $display("       16-VAULT TEST PASSED");
        $display("==============================================");

        $finish;

    end

endmodule

