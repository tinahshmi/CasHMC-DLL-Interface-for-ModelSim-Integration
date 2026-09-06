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


    import "DPI-C" function bit HMC_HasResponse();


    import "DPI-C" function bit HMC_GetResponse(
        output bit writeAck,
        output int unsigned tag,
        output longint unsigned address,
        output int unsigned bytes,
        output int unsigned vaultID
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



    int accepted_requests;
    int received_responses;

    int max_response_cycle;



    //============================================================
    // Prepare requests
    //============================================================

    task prepare_requests();

        for(int v=0; v<NUM_VAULTS; v++) begin


            req_address[v] =
                64'h1000 + v*64'h1000;


            case(v%8)

                0: req_bytes[v]=16;
                1: req_bytes[v]=32;
                2: req_bytes[v]=48;
                3: req_bytes[v]=64;
                4: req_bytes[v]=80;
                5: req_bytes[v]=96;
                6: req_bytes[v]=112;
                7: req_bytes[v]=128;

            endcase


            // even vaults WRITE
            // odd vaults READ

            req_write[v]=(v%2==0);


            completed[v]=0;


        end

    endtask



    //============================================================
    // Send all requests before simulation starts
    //============================================================

    task send_requests();


        accepted_requests=0;


        $display("");
        $display("===============================");
        $display(" Sending 16 simultaneous requests");
        $display("===============================");



        for(int v=0; v<NUM_VAULTS; v++) begin


            bit ok;


            if(req_write[v]) begin

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



            if(ok) begin

                accepted_requests++;


                $display(
                "Accepted Vault=%0d %s addr=0x%0h size=%0d",
                v,
                req_write[v]?"WRITE":"READ",
                req_address[v],
                req_bytes[v]
                );


            end
            else begin

                $display(
                "ERROR: request rejected vault %0d",
                v
                );

            end


        end



        if(accepted_requests != NUM_VAULTS) begin

            $fatal(
            1,
            "Not all requests accepted"
            );

        end


    endtask





    //============================================================
    // Check response
    //============================================================

    task check_response(
        input bit writeAck,
        input int unsigned tag,
        input longint unsigned address,
        input int unsigned bytes,
        input int unsigned vaultID
    );


        if(vaultID >= NUM_VAULTS) begin

            $fatal(
            1,
            "Invalid vault ID %0d",
            vaultID
            );

        end



        if(completed[vaultID]) begin

            $fatal(
            1,
            "Duplicate response from vault %0d",
            vaultID
            );

        end



        if(address != req_address[vaultID]) begin

            $fatal(
            1,
            "Wrong address from vault %0d",
            vaultID
            );

        end



        if(bytes != req_bytes[vaultID]) begin

            $fatal(
            1,
            "Wrong size from vault %0d",
            vaultID
            );

        end



        if(writeAck != req_write[vaultID]) begin

            $fatal(
            1,
            "Wrong command type from vault %0d",
            vaultID
            );

        end



        completed[vaultID]=1;


        $display(
        "VERIFIED: Vault=%0d TAG=%0d %s addr=0x%0h size=%0d",
        vaultID,
        tag,
        writeAck?"WRITE ACK":"READ",
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



        bit writeAck;

        int unsigned tag;

        int unsigned vaultID;

        longint unsigned address;

        int unsigned bytes;



        $display("");
        $display("===============================");
        $display(" CasHMC 16 Vault DPI TEST");
        $display("===============================");



        prepare_requests();



        init=HMC_Init();



        if(!init)

            $fatal(
            1,
            "HMC_Init failed"
            );



        send_requests();



        received_responses=0;

        max_response_cycle=0;



        for(int cycle=0; cycle<MAX_CYCLES; cycle++) begin



            responses_this_cycle=0;



            HMC_Update();



            while(HMC_HasResponse()) begin



                if (!HMC_GetResponse(
                        writeAck,
                        tag,
                        address,
                        bytes,
                        vaultID)) begin

                    $display("");
                    $display("ERROR: HMC_GetResponse() failed!");

                    HMC_Shutdown();
                    $fatal(1);

                end



                responses_this_cycle++;


                received_responses++;



                check_response(
                    writeAck,
                    tag,
                    address,
                    bytes,
                    vaultID
                );


            end



            if(responses_this_cycle>0) begin


                $display(
                "Cycle %0d : %0d responses",
                cycle,
                responses_this_cycle
                );


            end



            if(responses_this_cycle > max_response_cycle)

                max_response_cycle =
                    responses_this_cycle;



            if(received_responses==NUM_VAULTS)

                break;


        end




        $display("");
        $display("===============================");
        $display(" TEST SUMMARY");
        $display("===============================");

        $display(
        "Requests accepted : %0d",
        accepted_requests
        );


        $display(
        "Responses received: %0d",
        received_responses
        );


        $display(
        "Maximum responses/cycle: %0d",
        max_response_cycle
        );



        if(received_responses != NUM_VAULTS)

            $fatal(
            1,
            "Missing responses"
            );



        HMC_Shutdown();



        $display("");
        $display("===============================");
        $display(" 16 VAULT TEST PASSED");
        $display("===============================");



        $finish;


    end


endmodule