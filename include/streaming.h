// include/streaming.h - 스트리밍 헤더 (HTTP 전용)
#ifndef STREAMING_H
#define STREAMING_H

#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>

// 스트리밍 설정
struct StreamingConfig {
    bool websocket_enabled;
    bool http_streaming_enabled;
    unsigned long frame_interval;      // 프레임 전송 간격 (ms)
    unsigned long last_frame_time;
    int target_fps;
    size_t max_frame_size;
    bool streaming_active;
    int frame_count;
    unsigned long session_start;
};

// 전역 변수
extern StreamingConfig streaming_config;

// 스트리밍 관리 함수들 (HTTP 전용)
void initStreaming();
bool startHTTPStreaming();
void streamFrameToServer(camera_fb_t* fb);
void streamSensorDataToServer();
bool sendFrameViaHTTP(camera_fb_t* fb);
void processStreamingLoop();
void setStreamingFPS(int fps);
void printStreamingStats();

// 외부 함수 선언
extern bool isWiFiConnected();
extern bool sendSensorData();

#endif