

module tb;

    // ============================================================
    // CasHMC DPI interface
    // ============================================================

    import "DPI-C" function bit HMC_Init();
    import "DPI-C" function void HMC_Shutdown();
    import "DPI-C" function void HMC_Update();
    import "DPI-C" function void HMC_Reset();

    import "DPI-C" function bit HMC_Read(
        longint unsigned address,
        int unsigned bytes
    );

    import "DPI-C" function bit HMC_Write(
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

    longint unsigned TEST_ADDRESS = 64'h1000;
    int unsigned     TEST_BYTES   = 64;

    bit              writeAck;
    int unsigned     tag;
    longint unsigned responseAddress;
    int unsigned     responseBytes;

    bit init_ok;
    bit write_ok;
    bit read_ok;
    bit response_ok;

    int cycle;


    // ============================================================
    // Wait for a response
    // ============================================================

    task automatic wait_for_response(
        input string operation,
        input bit expected_write_ack
    );

        response_ok = 0;

        for (cycle = 0; cycle < 100; cycle++) begin

            $display("");
            $display("========== ModelSim Cycle %0d (%s) ==========",
                     cycle, operation);

            HMC_Update();

            if (HMC_HasResponse()) begin

                $display("");
                $display(">>> RESPONSE DETECTED at cycle %0d", cycle);

                writeAck      = 0;
                tag           = 0;
                responseAddress = 0;
                responseBytes = 0;

                if (HMC_GetResponse(
                        writeAck,
                        tag,
                        responseAddress,
                        responseBytes)) begin

                    $display("");
                    $display("========== %s RESPONSE ==========", operation);
                    $display("writeAck = %0d", writeAck);
                    $display("tag      = %0d", tag);
                    $display("address  = 0x%0h", responseAddress);
                    $display("bytes    = %0d", responseBytes);
                    $display("===================================");

                    // ------------------------------------------------
                    // Verify response
                    // ------------------------------------------------

                    if (writeAck !== expected_write_ack) begin
                        $display("");
                        $display("ERROR: Unexpected writeAck!");
                        $display("Expected = %0d", expected_write_ack);
                        $display("Received = %0d", writeAck);
                        $fatal(1);
                    end

                    if (responseAddress !== TEST_ADDRESS) begin
                        $display("");
                        $display("ERROR: Unexpected response address!");
                        $display("Expected = 0x%0h", TEST_ADDRESS);
                        $display("Received = 0x%0h", responseAddress);
                        $fatal(1);
                    end

                    if (responseBytes !== TEST_BYTES) begin
                        $display("");
                        $display("ERROR: Unexpected response size!");
                        $display("Expected = %0d", TEST_BYTES);
                        $display("Received = %0d", responseBytes);
                        $fatal(1);
                    end

                    $display("");
                    $display("%s RESPONSE VERIFIED",
                             operation);

                    response_ok = 1;
                    return;
                end
            end
        end

        $display("");
        $display("ERROR: No %s response received after 100 cycles!",
                 operation);

        $fatal(1);

    endtask


    // ============================================================
    // Main test
    // ============================================================

    initial begin

        $display("");
        $display("==============================================");
        $display("       CasHMC DPI WRITE + READ TEST");
        $display("==============================================");


        // ========================================================
        // 1. Initialize CasHMC
        // ========================================================

        $display("");
        $display("[1] HMC_Init()");

        init_ok = HMC_Init();

        if (!init_ok) begin
            $display("ERROR: HMC_Init() FAILED");
            $fatal(1);
        end

        $display("[2] HMC_Init() SUCCESS");


        // ========================================================
        // 2. WRITE
        // ========================================================

        $display("");
        $display("==============================================");
        $display("                 WRITE TEST");
        $display("==============================================");

        $display("");
        $display("[3] HMC_Write()");
        $display("    Address = 0x%0h", TEST_ADDRESS);
        $display("    Bytes   = %0d", TEST_BYTES);

        write_ok = HMC_Write(
            TEST_ADDRESS,
            TEST_BYTES
        );

        if (!write_ok) begin
            $display("ERROR: HMC_Write() FAILED");
            HMC_Shutdown();
            $fatal(1);
        end

        $display("[4] HMC_Write() SUCCESS");


        // ========================================================
        // 3. Wait for WRITE response
        // ========================================================

        wait_for_response(
            "WRITE",
            1'b1
        );


        // ========================================================
        // 4. READ
        // ========================================================

        $display("");
        $display("==============================================");
        $display("                  READ TEST");
        $display("==============================================");

        $display("");
        $display("[5] HMC_Read()");
        $display("    Address = 0x%0h", TEST_ADDRESS);
        $display("    Bytes   = %0d", TEST_BYTES);

        read_ok = HMC_Read(
            TEST_ADDRESS,
            TEST_BYTES
        );

        if (!read_ok) begin
            $display("ERROR: HMC_Read() FAILED");
            HMC_Shutdown();
            $fatal(1);
        end

        $display("[6] HMC_Read() SUCCESS");


        // ========================================================
        // 5. Wait for READ response
        // ========================================================

        wait_for_response(
            "READ",
            1'b0
        );


        // ========================================================
        // 6. Shutdown
        // ========================================================

        $display("");
        $display("==============================================");
        $display("             FINAL RESULT");
        $display("==============================================");

        HMC_Shutdown();

        $display("");
        $display("[7] HMC_Shutdown() SUCCESS");

        $display("");
        $display("==============================================");
        $display("       WRITE + READ TEST PASSED");
        $display("==============================================");

        $finish;

    end

endmodule


