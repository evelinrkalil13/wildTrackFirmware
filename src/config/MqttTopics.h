#pragma once
#include <cstdio>

struct MqttTopics {
    char events[80];
    char heartbeat[80];
    char status[80];
    char errors[80];
    char commands[80];

    void build(const char* device_id) {
        snprintf(events,    sizeof(events),    "wildtrack/devices/%s/events",    device_id);
        snprintf(heartbeat, sizeof(heartbeat), "wildtrack/devices/%s/heartbeat", device_id);
        snprintf(status,    sizeof(status),    "wildtrack/devices/%s/status",    device_id);
        snprintf(errors,    sizeof(errors),    "wildtrack/devices/%s/errors",    device_id);
        snprintf(commands,  sizeof(commands),  "wildtrack/devices/%s/commands",  device_id);
    }
};
