/* 
#include "web_server.h"
#include "camera_module.h"
#include "config.h"

WebServerModule webServer;

WebServerModule::WebServerModule() : wifiConnected(false) {}

bool WebServerModule::initWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("WiFi 연결 중");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi 연결됨. IP 주소: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
        return true;
    } else {
        Serial.println();
        Serial.println("WiFi 연결 실패!");
        return false;
    }
}

void WebServerModule::init() {
    setupRoutes();
    server.begin();
    Serial.println("웹서버 시작됨");
}

void WebServerModule::setupRoutes() {
    server.on("/", handleRoot);
    server.on("/capture", handleCapture);
    server.on("/sensors", handleSensors);
    server.on("/data", handleDataPage);
    server.onNotFound(handleNotFound);
}

void WebServerModule::handleClient() {
    server.handleClient();
}

void WebServerModule::handleRoot() {
    server.send(200, "text/html", getMainPageHTML());
}

void WebServerModule::handleCapture() {
    // 임시로 카메라 대신 더미 응답
    server.send(200, "text/plain", "카메라 기능 준비 중...");
    
    /* 원래 코드 (나중에 활성화)
    camera_fb_t* fb = camera.captureImage();
    if (!fb) {
        server.send(500, "text/plain", "카메라 캡처 실패");
        return;
    }
    
    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Length", String(fb->len));
    server.sendHeader("Cache-Control", "no-cache");
    server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
    
    camera.releaseFrameBuffer(fb);
}

void WebServerModule::handleSensors() {
    StaticJsonDocument<300> doc;
    
    if (currentSensorData.isValid) {
        doc["temperature"] = currentSensorData.temperature;
        doc["accelX"] = currentSensorData.accelX;
        doc["accelY"] = currentSensorData.accelY;
        doc["accelZ"] = currentSensorData.accelZ;
        doc["gyroX"] = currentSensorData.gyroX;
        doc["gyroY"] = currentSensorData.gyroY;
        doc["gyroZ"] = currentSensorData.gyroZ;
        doc["timestamp"] = currentSensorData.timestamp;
        doc["status"] = "ok";
    } else {
        doc["status"] = "error";
        doc["message"] = "센서 데이터를 읽을 수 없습니다";
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
}

void WebServerModule::handleDataPage() {
    server.send(200, "text/html", getDataPageHTML());
}

void WebServerModule::handleNotFound() {
    server.send(404, "text/plain", "페이지를 찾을 수 없습니다");
}

String WebServerModule::getMainPageHTML() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>T-Camera S3 Multi-Sensor</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 1000px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; margin-bottom: 30px; }
        .camera-section { text-align: center; margin: 30px 0; padding: 20px; background: #f8f9fa; border-radius: 8px; }
        .sensor-section { margin: 30px 0; padding: 20px; background: #f8f9fa; border-radius: 8px; }
        img { max-width: 100%; height: auto; border: 2px solid #ddd; border-radius: 8px; }
        button { padding: 10px 20px; margin: 5px; background: #007bff; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 14px; }
        button:hover { background: #0056b3; }
        .sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin-top: 15px; }
        .sensor-item { background: white; padding: 15px; border-radius: 5px; border-left: 4px solid #007bff; }
        .sensor-value { font-size: 1.2em; font-weight: bold; color: #333; }
        .sensor-label { color: #666; font-size: 0.9em; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🎥 T-Camera S3 Multi-Sensor Dashboard</h1>
            <p>실시간 카메라 및 센서 모니터링 시스템</p>
        </div>
        
        <div class="camera-section">
            <h2>📸 카메라 모니터링</h2>
            <img id="camera" src="/capture" alt="카메라 이미지" style="max-height: 400px;">
            <br><br>
            <button onclick="refreshCamera()">📷 이미지 새로고침</button>
            <button onclick="toggleAutoRefresh()">🔄 자동 새로고침 토글</button>
        </div>
        
        <div class="sensor-section">
            <h2>📊 센서 데이터</h2>
            <div id="sensorData">
                <div class="sensor-grid">
                    <div class="sensor-item">
                        <div class="sensor-label">온도</div>
                        <div class="sensor-value" id="temp">-- °C</div>
                    </div>
                    <div class="sensor-item">
                        <div class="sensor-label">가속도 X</div>
                        <div class="sensor-value" id="accelX">-- m/s²</div>
                    </div>
                    <div class="sensor-item">
                        <div class="sensor-label">가속도 Y</div>
                        <div class="sensor-value" id="accelY">-- m/s²</div>
                    </div>
                    <div class="sensor-item">
                        <div class="sensor-label">가속도 Z</div>
                        <div class="sensor-value" id="accelZ">-- m/s²</div>
                    </div>
                    <div class="sensor-item">
                        <div class="sensor-label">자이로 X</div>
                        <div class="sensor-value" id="gyroX">-- rad/s</div>
                    </div>
                    <div class="sensor-item">
                        <div class="sensor-label">자이로 Y</div>
                        <div class="sensor-value" id="gyroY">-- rad/s</div>
                    </div>
                    <div class="sensor-item">
                        <div class="sensor-label">자이로 Z</div>
                        <div class="sensor-value" id="gyroZ">-- rad/s</div>
                    </div>
                    <div class="sensor-item">
                        <div class="sensor-label">업데이트 시간</div>
                        <div class="sensor-value" id="updateTime">--</div>
                    </div>
                </div>
            </div>
            <br>
            <button onclick="refreshSensors()">🔄 센서 데이터 새로고침</button>
            <button onclick="location.href='/data'">📈 실시간 모니터링 페이지</button>
        </div>
    </div>

    <script>
        let autoRefreshCamera = true;
        let autoRefreshSensors = true;
        
        function refreshCamera() {
            const img = document.getElementById('camera');
            img.src = '/capture?' + new Date().getTime();
        }
        
        function refreshSensors() {
            fetch('/sensors')
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'ok') {
                        document.getElementById('temp').textContent = data.temperature.toFixed(2) + ' °C';
                        document.getElementById('accelX').textContent = data.accelX.toFixed(2) + ' m/s²';
                        document.getElementById('accelY').textContent = data.accelY.toFixed(2) + ' m/s²';
                        document.getElementById('accelZ').textContent = data.accelZ.toFixed(2) + ' m/s²';
                        document.getElementById('gyroX').textContent = data.gyroX.toFixed(2) + ' rad/s';
                        document.getElementById('gyroY').textContent = data.gyroY.toFixed(2) + ' rad/s';
                        document.getElementById('gyroZ').textContent = data.gyroZ.toFixed(2) + ' rad/s';
                        document.getElementById('updateTime').textContent = new Date(data.timestamp).toLocaleTimeString();
                    } else {
                        console.error('센서 데이터 오류:', data.message);
                    }
                })
                .catch(error => console.error('센서 데이터 로드 실패:', error));
        }
        
        function toggleAutoRefresh() {
            autoRefreshCamera = !autoRefreshCamera;
            autoRefreshSensors = !autoRefreshSensors;
            alert('자동 새로고침: ' + (autoRefreshCamera ? '활성화' : '비활성화'));
        }
        
        // 자동 새로고침
        setInterval(() => {
            if (autoRefreshSensors) refreshSensors();
        }, 2000);
        
        setInterval(() => {
            if (autoRefreshCamera) refreshCamera();
        }, 5000);
        
        // 초기 로드
        refreshSensors();
    </script>
</body>
</html>
)";
}

String WebServerModule::getDataPageHTML() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>실시간 센서 데이터</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #1e3c72, #2a5298); color: white; min-height: 100vh; }
        .container { max-width: 1200px; margin: 0 auto; }
        .header { text-align: center; margin-bottom: 40px; }
        .status { text-align: center; margin: 20px 0; padding: 10px; border-radius: 5px; }
        .status.connected { background: rgba(0, 255, 0, 0.2); border: 1px solid #00ff00; }
        .status.error { background: rgba(255, 0, 0, 0.2); border: 1px solid #ff0000; }
        .data-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 25px; }
        .data-card { background: rgba(255, 255, 255, 0.1); backdrop-filter: blur(10px); padding: 25px; border-radius: 15px; border: 1px solid rgba(255, 255, 255, 0.2); transition: transform 0.3s ease; }
        .data-card:hover { transform: translateY(-5px); }
        .label { color: #b3d9ff; margin-bottom: 15px; font-size: 1.1em; font-weight: 500; }
        .value { font-size: 2.5em; font-weight: bold; color: #00ff88; text-shadow: 0 0 10px rgba(0, 255, 136, 0.5); margin-bottom: 10px; }
        .unit { color: #ccc; font-size: 1.1em; }
        .back-button { position: fixed; top: 20px; left: 20px; background: rgba(255, 255, 255, 0.2); border: none; color: white; padding: 10px 15px; border-radius: 8px; cursor: pointer; }
        .back-button:hover { background: rgba(255, 255, 255, 0.3); }
    </style>
</head>
<body>
    <button class="back-button" onclick="history.back()">← 뒤로</button>
    
    <div class="container">
        <div class="header">
            <h1>🚀 T-Camera S3 실시간 모니터링</h1>
            <p>Live Sensor Data Dashboard</p>
        </div>
        
        <div class="status" id="status">연결 중...</div>
        
        <div class="data-grid">
            <div class="data-card">
                <div class="label">🌡️ 온도</div>
                <div class="value" id="temp">--</div>
                <div class="unit">°C</div>
            </div>
            <div class="data-card">
                <div class="label">📊 가속도 X</div>
                <div class="value" id="accelX">--</div>
                <div class="unit">m/s²</div>
            </div>
            <div class="data-card">
                <div class="label">📊 가속도 Y</div>
                <div class="value" id="accelY">--</div>
                <div class="unit">m/s²</div>
            </div>
            <div class="data-card">
                <div class="label">📊 가속도 Z</div>
                <div class="value" id="accelZ">--</div>
                <div class="unit">m/s²</div>
            </div>
            <div class="data-card">
                <div class="label">🌀 자이로 X</div>
                <div class="value" id="gyroX">--</div>
                <div class="unit">rad/s</div>
            </div>
            <div class="data-card">
                <div class="label">🌀 자이로 Y</div>
                <div class="value" id="gyroY">--</div>
                <div class="unit">rad/s</div>
            </div>
            <div class="data-card">
                <div class="label">🌀 자이로 Z</div>
                <div class="value" id="gyroZ">--</div>
                <div class="unit">rad/s</div>
            </div>
            <div class="data-card">
                <div class="label">⏰ 마지막 업데이트</div>
                <div class="value" id="lastUpdate" style="font-size: 1.5em;">--</div>
                <div class="unit">시간</div>
            </div>
        </div>
    </div>

    <script>
        function updateData() {
            fetch('/sensors')
                .then(response => response.json())
                .then(data => {
                    const statusEl = document.getElementById('status');
                    
                    if (data.status === 'ok') {
                        statusEl.textContent = '✅ 연결됨 - 실시간 데이터 수신 중';
                        statusEl.className = 'status connected';
                        
                        document.getElementById('temp').textContent = data.temperature.toFixed(2);
                        document.getElementById('accelX').textContent = data.accelX.toFixed(2);
                        document.getElementById('accelY').textContent = data.accelY.toFixed(2);
                        document.getElementById('accelZ').textContent = data.accelZ.toFixed(2);
                        document.getElementById('gyroX').textContent = data.gyroX.toFixed(2);
                        document.getElementById('gyroY').textContent = data.gyroY.toFixed(2);
                        document.getElementById('gyroZ').textContent = data.gyroZ.toFixed(2);
                        document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
                    } else {
                        statusEl.textContent = '❌ 센서 오류: ' + (data.message || '알 수 없는 오류');
                        statusEl.className = 'status error';
                    }
                })
                .catch(error => {
                    const statusEl = document.getElementById('status');
                    statusEl.textContent = '🔌 연결 끊김 - 재연결 시도 중...';
                    statusEl.className = 'status error';
                    console.error('데이터 로드 실패:', error);
                });
        }
        
        // 1초마다 데이터 업데이트
        setInterval(updateData, 1000);
        
        // 초기 로드
        updateData();
    </script>
</body>
</html>
)";
}
*/