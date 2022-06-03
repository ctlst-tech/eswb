#ifndef ESWB_CTL_H
#define ESWB_CTL_H

#include <eswb/api.h>

eswb_rv_t eswb_ctl(eswb_topic_descr_t td, eswb_ctl_t ctl_type, void *d, int size);

#endif //ESWB_CTL_H
