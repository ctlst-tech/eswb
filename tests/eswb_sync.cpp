#include "tooling.h"

#include "eswb/api_sync.h"

// TODO never tested, suspended

void alter_buffer(const uint8_t *in, int in_l, uint8_t *out, int out_l) {
    uint8_t sum = 0;
    for (int i = 0; i < in_l; i++){
        sum += in[i];
    }

    int j = 0;
    for (int i = 0; i < out_l; i++) {
        out[i] = in[j] & sum;
        j++;
        if (j >= in_l) {
            j = 0;
        }
    }
}

void generate_data(uint8_t *out, int out_l) {
    for (int i = 0; i < out_l; i++) {
        out[i] = i;
    }
}

TEST_CASE("Sync interaction") {
    eswb_local_init(1);

    eswb_sync_handle_t server_ch;
    eswb_sync_handle_t client_ch;

    std::string channel_path = "test_channel";

    eswb_rv_t rv;

    int req_size = 32;
    int resp_size = 64;

    rv = eswb_s_channel_create(channel_path.c_str(), req_size, resp_size, &server_ch);
    REQUIRE(rv == eswb_e_ok);

    periodic_call_t server = [&] (){
        uint8_t req_buffer[req_size];
        uint8_t resp_buffer[resp_size];

        eswb_rv_t erv = eswb_s_receive(&server_ch, (void *)req_buffer);
        REQUIRE(erv == eswb_e_ok);

        alter_buffer(req_buffer, req_size, resp_buffer, resp_size);

        erv = eswb_s_reply(&server_ch, resp_buffer);
        REQUIRE(erv == eswb_e_ok);
    };

    timed_caller server_thread(server, 0);

    SECTION("Simple connection") {
        rv = eswb_s_channel_connect(channel_path.c_str(), req_size, resp_size, &client_ch);
        REQUIRE(rv == eswb_e_ok);
    }

    SECTION("Violated size params") {
        rv = eswb_s_channel_connect(channel_path.c_str(), req_size+1, resp_size-1, &client_ch);
        REQUIRE(rv != eswb_e_ok);
    }

    rv = eswb_s_channel_connect(channel_path.c_str(), req_size, resp_size, &client_ch);
    REQUIRE(rv != eswb_e_ok);

    SECTION("Single interaction") {
        server_thread.start_once();
        uint8_t req_buffer[req_size];
        uint8_t resp_buffer[resp_size];
        uint8_t reference_buffer[resp_size];

        memset(resp_buffer, 0, sizeof (resp_buffer));

        generate_data(req_buffer, req_size);

        rv = eswb_s_request(&client_ch, req_buffer, resp_buffer);
        REQUIRE(rv == eswb_e_ok);

        alter_buffer(req_buffer, req_size, reference_buffer, resp_size);
        REQUIRE(memcmp(resp_buffer, reference_buffer, resp_size) == 0);
    }
}
