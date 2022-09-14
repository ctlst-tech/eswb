#include <string>
#include "sdtl_tooling.h"



struct test_media_store_handle {
    SDTLtestBridge *bridge_ref;
//    ThreadSafeQueue<SDTLtestPacket> &stream;
    bool up_agent;

    test_media_store_handle(SDTLtestBridge *br, const std::string &path) : bridge_ref(br) {
        up_agent = path == "up";
    }
};


sdtl_rv_t sdtl_test_media_open(const char *path, void *params, void **h_rv) {
    auto *br_ref = (SDTLtestBridge *) params;
    auto *h = new test_media_store_handle(br_ref, path);

    *h_rv = h;

    return SDTL_OK;
}

sdtl_rv_t sdtl_test_media_close(void *h) {
    // TODO
    return SDTL_MEDIA_ERR;
}

sdtl_rv_t sdtl_test_media_read(void *h, void *data, size_t l, size_t *lr) {
    auto *bh = (test_media_store_handle *)h;
    *lr = bh->bridge_ref->read(bh->up_agent, data, l);
    return *lr == 0 ? SDTL_MEDIA_EOF : SDTL_OK;
}

sdtl_rv_t sdtl_test_media_write(void *h, void *data, size_t l) {
    auto *bh = (test_media_store_handle *)h;
    bh->bridge_ref->write(bh->up_agent, data, l);
    return SDTL_OK;
}


const sdtl_service_media_t sdtl_test_media = {
        .open = sdtl_test_media_open,
        .read = sdtl_test_media_read,
        .write = sdtl_test_media_write,
        .close = sdtl_test_media_close
};
