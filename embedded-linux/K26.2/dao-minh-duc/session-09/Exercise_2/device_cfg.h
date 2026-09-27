#ifndef DEVICE_CFG_H
#define DEVICE_CFG_H

#define CONFIG_FILE_PATH "/tmp/device.cfg"

typedef struct {
    int baud_rate;
    int sampling_rate_hz;
    int log_level; /* 0=OFF, 1=ERROR, 2=INFO, 3=DEBUG */
} device_cfg_t;

#endif // DEVICE_CFG_H
