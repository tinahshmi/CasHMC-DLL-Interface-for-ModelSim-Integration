module tb;


    //============================================================
    // DPI Interface
    //============================================================

    import "DPI-C" function bit HMC_Init();

    import "DPI-C" function void HMC_Shutdown();

    import "DPI-C" function void HMC_Update();


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


    // Check response availability for a specific vault.
    import "DPI-C" function bit HMC_HasResponse(
        int unsigned vaultID
    );


    // Get one response from a specific vault.
    import "DPI-C" function bit HMC_GetResponse(
        int unsigned vaultID,
        output bit writeAck,
        output int unsigned tag,
        output longint unsigned address,
        output int unsigned bytes,
        output int unsigned responseVaultID
    );


    //============================================================
    // Parameters
    //============================================================

    localparam int NUM_VAULTS = 16;

    localparam int MAX_CYCLES = 1000;


    //============================================================
    // Request information
    //============================================================

    longint unsigned req_address [NUM_VAULTS];

    int unsigned req_bytes [NUM_VAULTS];

    bit req_write [NUM_VAULTS];

    bit completed [NUM_VAULTS];


    //============================================================
    // Response state tracking
    //
    // has_response_previous[v]
    //     Previous value of HMC_HasResponse(v)
    //
    // This allows us to detect:
    //
    //     0 -> 1 : response becomes ready
    //
    //     1 -> 0 : response queue becomes empty
    //============================================================

    bit has_response_previous [NUM_VAULTS];


    //============================================================
    // Test counters
    //============================================================

    int accepted_requests;

    int received_responses;

    int max_response_cycle;


    //============================================================
    // Prepare requests
    //============================================================

    task prepare_requests();

        for (int v = 0; v < NUM_VAULTS; v++) begin

            // Each vault gets a different address.
            req_address[v] =
                64'h1000 + v * 64'h1000;


            // Use all supported request sizes.
            case (v % 8)

                0: req_bytes[v] = 16;
                1: req_bytes[v] = 32;
                2: req_bytes[v] = 48;
                3: req_bytes[v] = 64;
                4: req_bytes[v] = 80;
                5: req_bytes[v] = 96;
                6: req_bytes[v] = 112;
                7: req_bytes[v] = 128;

            endcase


            // Even vaults: WRITE
            // Odd vaults : READ
            req_write[v] = (v % 2 == 0);


            completed[v] = 0;


            // Initially no response is available.
            has_response_previous[v] = 0;

        end

    endtask


    //============================================================
    // Send all requests
    //============================================================

    task send_requests();

        accepted_requests = 0;


        $display("");
        $display("==============================================");
        $display(" Sending 16 simultaneous requests");
        $display("==============================================");


        for (int v = 0; v < NUM_VAULTS; v++) begin

            bit ok;


            if (req_write[v]) begin

                ok = HMC_Write(
                    v,
                    req_address[v],
                    req_bytes[v]
                );

            end
            else begin

                ok = HMC_Read(
                    v,
                    req_address[v],
                    req_bytes[v]
                );

            end


            if (ok) begin

                accepted_requests++;


                $display(
                    "Accepted Vault=%0d %s addr=0x%0h size=%0d",
                    v,
                    req_write[v] ? "WRITE" : "READ",
                    req_address[v],
                    req_bytes[v]
                );

            end
            else begin

                $display(
                    "ERROR: request rejected for vault %0d",
                    v
                );

            end

        end


        if (accepted_requests != NUM_VAULTS) begin

            $fatal(
                1,
                "Not all requests were accepted: %0d/%0d",
                accepted_requests,
                NUM_VAULTS
            );

        end


        $display("");
        $display(
            "All %0d requests accepted.",
            accepted_requests
        );

    endtask


    //============================================================
    // Check one response
    //============================================================

    task check_response(
        input int unsigned queriedVault,
        input bit writeAck,
        input int unsigned tag,
        input longint unsigned address,
        input int unsigned bytes,
        input int unsigned responseVaultID
    );


        // -------------------------------------------------------
        // Check returned vault ID
        // -------------------------------------------------------

        if (responseVaultID >= NUM_VAULTS) begin

            $fatal(
                1,
                "Invalid responseVaultID %0d",
                responseVaultID
            );

        end


        // -------------------------------------------------------
        // Check that the response came from the vault queried
        // by HMC_GetResponse().
        // -------------------------------------------------------

        if (responseVaultID != queriedVault) begin

            $fatal(
                1,
                "VAULT MISMATCH: queried vault=%0d, response vault=%0d",
                queriedVault,
                responseVaultID
            );

        end


        // -------------------------------------------------------
        // Check duplicate response
        // -------------------------------------------------------

        if (completed[responseVaultID]) begin

            $fatal(
                1,
                "Duplicate response from vault %0d",
                responseVaultID
            );

        end


        // -------------------------------------------------------
        // Check address
        // -------------------------------------------------------

        if (address != req_address[responseVaultID]) begin

            $fatal(
                1,
                "Wrong address from vault %0d: expected=0x%0h got=0x%0h",
                responseVaultID,
                req_address[responseVaultID],
                address
            );

        end


        // -------------------------------------------------------
        // Check request size
        // -------------------------------------------------------

        if (bytes != req_bytes[responseVaultID]) begin

            $fatal(
                1,
                "Wrong size from vault %0d: expected=%0d got=%0d",
                responseVaultID,
                req_bytes[responseVaultID],
                bytes
            );

        end


        // -------------------------------------------------------
        // Check operation type
        //
        // WRITE -> writeAck = 1
        // READ  -> writeAck = 0
        // -------------------------------------------------------

        if (writeAck != req_write[responseVaultID]) begin

            $fatal(
                1,
                "Wrong command type from vault %0d",
                responseVaultID
            );

        end


        // -------------------------------------------------------
        // Mark request as completed
        // -------------------------------------------------------

        completed[responseVaultID] = 1;


        $display(
            "    VERIFIED: QueryVault=%0d ResponseVault=%0d TAG=%0d %s addr=0x%0h size=%0d",
            queriedVault,
            responseVaultID,
            tag,
            writeAck ? "WRITE ACK" : "READ",
            address,
            bytes
        );


    endtask


    //============================================================
    // Main simulation
    //============================================================

    initial begin


        bit init;


        int responses_this_cycle;

        int available_vaults;


        bit writeAck;

        int unsigned tag;

        int unsigned responseVaultID;

        longint unsigned address;

        int unsigned bytes;


        //========================================================
        // Test header
        //========================================================

        $display("");

        $display("==============================================");

        $display(" CasHMC 16-Vault DPI Test");

        $display(" Per-Vault Response Queue Verification");

        $display(" Response Ready / Get / Empty Tracking");

        $display("==============================================");


        //========================================================
        // Prepare requests
        //========================================================

        prepare_requests();


        //========================================================
        // Initialize CasHMC
        //========================================================

        init = HMC_Init();


        if (!init) begin

            $fatal(
                1,
                "HMC_Init failed"
            );

        end


        $display("");

        $display("HMC_Init() SUCCESS");


        //========================================================
        // Send all requests
        //========================================================

        send_requests();


        //========================================================
        // Initialize counters
        //========================================================

        received_responses = 0;

        max_response_cycle = 0;


        //========================================================
        // Main simulation loop
        //========================================================

        for (int cycle = 0;
             cycle < MAX_CYCLES;
             cycle++) begin


            responses_this_cycle = 0;

            available_vaults = 0;


            //====================================================
            // Advance CasHMC by one update
            //====================================================

            HMC_Update();


            //====================================================
            // Check all vaults for newly available responses
            //====================================================

            for (int v = 0;
                 v < NUM_VAULTS;
                 v++) begin


                bit has_response;


                has_response = HMC_HasResponse(v);


                // ------------------------------------------------
                // Detect response becoming READY:
                //
                // Previous = 0
                // Current  = 1
                //
                // This is the exact point at which the response
                // becomes visible to the testbench.
                // ------------------------------------------------

                if (!has_response_previous[v] &&
                    has_response) begin


                    $display("");

                    $display(
                        ">>> RESPONSE READY: cycle=%0d vault=%0d",
                        cycle,
                        v
                    );

                    $display(
                        "    HMC_HasResponse(%0d): 0 -> 1",
                        v
                    );

                    $display(
                        "    Request: %s addr=0x%0h size=%0d",
                        req_write[v] ? "WRITE" : "READ",
                        req_address[v],
                        req_bytes[v]
                    );

                    $display(
                        "    Next action: testbench will call HMC_GetResponse(%0d)",
                        v
                    );


                end


                if (has_response)

                    available_vaults++;


                has_response_previous[v] = has_response;

            end


            //====================================================
            // Get responses
            //====================================================
            //
            // We now check every vault independently.
            //
            // If HMC_HasResponse(v) is TRUE, we explicitly show
            // that the testbench is about to call GetResponse(v).
            //
            //====================================================

            for (int v = 0;
                 v < NUM_VAULTS;
                 v++) begin


                bit has_response;


                has_response = HMC_HasResponse(v);


                if (!has_response)

                    continue;


                // ------------------------------------------------
                // EXACT MOMENT:
                // Testbench decides to retrieve the response.
                // ------------------------------------------------

                $display("");

                $display(
                    ">>> GET RESPONSE: cycle=%0d vault=%0d",
                    cycle,
                    v
                );

                $display(
                    "    Calling HMC_GetResponse(%0d, ...)",
                    v
                );

                $display(
                    "    HMC_HasResponse(%0d) BEFORE GetResponse = %0d",
                    v,
                    has_response
                );


                // ------------------------------------------------
                // Get the response.
                // ------------------------------------------------

                if (!HMC_GetResponse(
                        v,
                        writeAck,
                        tag,
                        address,
                        bytes,
                        responseVaultID
                    )) begin


                    $display("");

                    $display(
                        "ERROR: HMC_GetResponse(%0d) failed!",
                        v
                    );


                    HMC_Shutdown();


                    $fatal(
                        1,
                        "HMC_GetResponse failed for vault %0d",
                        v
                    );

                end


                responses_this_cycle++;

                received_responses++;


                // ------------------------------------------------
                // Show returned response information.
                // ------------------------------------------------

                $display(
                    "    HMC_GetResponse(%0d) RETURNED SUCCESS",
                    v
                );

                $display(
                    "    Returned: ResponseVault=%0d TAG=%0d addr=0x%0h size=%0d",
                    responseVaultID,
                    tag,
                    address,
                    bytes
                );


                // ------------------------------------------------
                // Verify response.
                // ------------------------------------------------

                check_response(
                    v,
                    writeAck,
                    tag,
                    address,
                    bytes,
                    responseVaultID
                );


                // ------------------------------------------------
                // Check HasResponse immediately AFTER
                // GetResponse().
                //
                // Because there is exactly one request per vault,
                // the queue must now be empty.
                // ------------------------------------------------

                has_response = HMC_HasResponse(v);


                $display(
                    "    HMC_HasResponse(%0d) AFTER GetResponse = %0d",
                    v,
                    has_response
                );


                if (has_response) begin

                    $fatal(
                        1,
                        "ERROR: Vault %0d still has a response after GetResponse()",
                        v
                    );

                end


                // ------------------------------------------------
                // Explicit 1 -> 0 transition message.
                // ------------------------------------------------

                $display(
                    "<<< RESPONSE CONSUMED: cycle=%0d vault=%0d",
                    cycle,
                    v
                );

                $display(
                    "    HMC_HasResponse(%0d): 1 -> 0",
                    v
                );

                $display(
                    "    Vault %0d response queue is now EMPTY",
                    v
                );


                // ------------------------------------------------
                // Update previous state.
                // ------------------------------------------------

                has_response_previous[v] = 0;


            end


            //====================================================
            // Cycle summary
            //====================================================

            if (responses_this_cycle > 0) begin

                $display("");

                $display(
                    "------------------------------------------------"
                );

                $display(
                    "Cycle %0d SUMMARY: available=%0d released=%0d total=%0d",
                    cycle,
                    available_vaults,
                    responses_this_cycle,
                    received_responses
                );

                $display(
                    "------------------------------------------------"
                );

            end


            //====================================================
            // Track maximum responses in one cycle
            //====================================================

            if (responses_this_cycle > max_response_cycle)

                max_response_cycle =
                    responses_this_cycle;


            //====================================================
            // All responses received?
            //====================================================

            if (received_responses == NUM_VAULTS)

                break;


        end


        //============================================================
        // Final verification
        //============================================================

        $display("");

        $display("==============================================");

        $display(" Final Verification");

        $display("==============================================");


        for (int v = 0;
             v < NUM_VAULTS;
             v++) begin


            // ----------------------------------------------------
            // Every vault must have completed.
            // ----------------------------------------------------

            if (!completed[v]) begin

                $fatal(
                    1,
                    "Vault %0d did not receive a response",
                    v
                );

            end


            // ----------------------------------------------------
            // Every vault queue must be empty.
            // ----------------------------------------------------

            if (HMC_HasResponse(v)) begin

                $fatal(
                    1,
                    "Vault %0d response queue is not empty",
                    v
                );

            end


            $display(
                "Vault %0d: completed=1, HMC_HasResponse=%0d",
                v,
                HMC_HasResponse(v)
            );

        end


        //============================================================
        // Test summary
        //============================================================

        $display("");

        $display("==============================================");

        $display(" TEST SUMMARY");

        $display("==============================================");


        $display(
            "Requests accepted        : %0d",
            accepted_requests
        );


        $display(
            "Responses received       : %0d",
            received_responses
        );


        $display(
            "Maximum responses/cycle  : %0d",
            max_response_cycle
        );


        $display(
            "Completed vaults         : %0d/%0d",
            received_responses,
            NUM_VAULTS
        );


        //============================================================
        // Final checks
        //============================================================

        if (accepted_requests != NUM_VAULTS) begin

            $fatal(
                1,
                "TEST FAILED: not all requests were accepted"
            );

        end


        if (received_responses != NUM_VAULTS) begin

            $fatal(
                1,
                "TEST FAILED: missing responses"
            );

        end


        //============================================================
        // Shutdown
        //============================================================

        HMC_Shutdown();


        $display("");

        $display("HMC_Shutdown() SUCCESS");


        //============================================================
        // PASS
        //============================================================

        $display("");

        $display("==============================================");

        $display(" 16-VAULT PER-VAULT DPI TEST PASSED");

        $display("==============================================");


        $finish;


    end


endmodule