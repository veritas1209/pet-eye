// include/pet_eye_streaming.h - 펫아이 실시간 스트리밍
#ifndef PET_EYE_STREAMING_H
#define PET_EYE_STREAMING_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include "esp_camera.h"

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

// WebSocket 이벤트 타입
enum WebSocketEventType {
    WS_FRAME_DATA,
    WS_SENSOR_DATA,
    WS_STATUS_UPDATE,
    WS_COMMAND_RESPONSE
};

// 전역 변수
extern StreamingConfig streaming_config;
extern WebSocketsClient webSocket;

// 스트리밍 관리 함수들
void initStreaming();
bool startWebSocketStreaming();
void stopWebSocketStreaming();
void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
void streamFrameToServer(camera_fb_t* fb);
void streamSensorDataToServer();
bool sendFrameViaHTTP(camera_fb_t* fb);
bool sendFrameViaWebSocket(camera_fb_t* fb);
void processStreamingLoop();
void setStreamingFPS(int fps);
void printStreamingStats();

// 명령 처리 함수들
void processWebSocketCommand(String command);
void sendCommandResponse(String command, String result);

#endif