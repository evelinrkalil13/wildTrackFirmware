#include "MediaUploadService.h"
#include <HTTPClient.h>

bool MediaUploadService::upload(const char* host, uint16_t port,
                                const char* deviceId, const char* eventId,
                                camera_fb_t* fb,
                                char* outUrl, size_t urlLen) {
    if (!fb || !fb->buf || fb->len == 0) return false;

    char url[300];
    snprintf(url, sizeof(url),
             "http://%s:%u/api/v1/media/upload?device_id=%s&event_id=%s",
             host, port, deviceId, eventId);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "image/jpeg");
    int code = http.POST(fb->buf, fb->len);

    if (code == 201) {
        String body = http.getString();
        int start = body.indexOf("\"url\":\"");
        if (start >= 0) {
            start += 7;
            int end = body.indexOf("\"", start);
            if (end > start) {
                String extracted = body.substring(start, end);
                strncpy(outUrl, extracted.c_str(), urlLen - 1);
                outUrl[urlLen - 1] = '\0';
                http.end();
                Serial.print("[Media] Subida OK: "); Serial.println(outUrl);
                return true;
            }
        }
    }

    Serial.print("[Media] Fallo HTTP "); Serial.println(code);
    http.end();
    return false;
}
