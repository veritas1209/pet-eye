// include/http_client.h - HTTP 통신 관리
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include "esp_camera.h"

// 전역 HTTP 클라이언트
extern HTTPClient http;

// HTTP 관리 함수들
String createSensorJSON();
bool sendSensorData();
bool sendCameraImage(camera_fb_t* fb);
bool sendImageAndSensorData(camera_fb_t* fb);

#endif