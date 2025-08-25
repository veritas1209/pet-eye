#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "esp_camera.h"
#include "config.h"
#include "debug_system.h"

class CameraManager {
public:
    static bool init();
    static camera_fb_t* capture();
    static void releaseFrame(camera_fb_t* fb);
    static bool isInitialized();
    static bool testCapture();
};

#endif // CAMERA_MANAGER_H