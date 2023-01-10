#include <catch2/catch_all.hpp>

#include "../src/lib/include/public/eswb/api.h"
#include "../src/lib/services/eqrb/eqrb_priv.h"

extern const eqrb_media_driver_t eqrb_drv_file;

class EqrbFileTest {
public:
    EqrbFileTest() = default;

    eqrb_rv_t createPublisherThread() {
        eqrb_rv_t rv;
        return rv;
    }

    eqrb_rv_t startService(const char *service_name, const char *bus2replicate,
                           const char *file_prefix, const char *dst_dir) {
        const char *err_msg;
        if (service_name == NULL || bus2replicate == NULL ||
            file_prefix == NULL || dst_dir == NULL) {
            return eqrb_invarg;
        }

        eqrb_rv_t rv = eqrb_file_server_start(service_name, file_prefix,
                                              dst_dir, bus2replicate, &err_msg);

        return rv;
    }
};
