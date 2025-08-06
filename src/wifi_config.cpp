// src/wifi_config.cpp - WiFi 설정 관리 구현
#include "wifi_config.h"
#include "config.h"

// 전역 변수 정의
WiFiCredentials wifi_config;
ConfigMode current_mode = CONFIG_MODE_NORMAL;
WebServer config_server(80);
DNSServer dns_server;
Preferences preferences;

// AP 모드 설정
const char* AP_SSID = "T-Camera-Config";
const char* AP_PASSWORD = "12345678";
const int CONFIG_TIMEOUT = 300000; // 5분

void initWiFiConfig() {
    Serial.println("WiFi 설정 시스템 초기화...");
    
    // Preferences 초기화
    preferences.begin("wifi_config", false);
    
    // 저장된 설정 로드
    if (loadWiFiConfig()) {
        Serial.println("✅ 저장된 WiFi 설정 로드 완료");
        printWiFiConfig();
        current_mode = CONFIG_MODE_NORMAL;
    } else {
        Serial.println("⚠️  저장된 WiFi 설정 없음 - 설정 모드 시작");
        current_mode = CONFIG_MODE_AP;
        startConfigPortal();
    }
}

bool loadWiFiConfig() {
    wifi_config.ssid = preferences.getString("ssid", "");
    wifi_config.password = preferences.getString("password", "");
    wifi_config.server_url = preferences.getString("server_url", "http://192.168.0.27:3000/api/sensors");
    wifi_config.is_configured = preferences.getBool("configured", false);
    
    return wifi_config.is_configured && wifi_config.ssid.length() > 0;
}

void saveWiFiConfig() {
    preferences.putString("ssid", wifi_config.ssid);
    preferences.putString("password", wifi_config.password);
    preferences.putString("server_url", wifi_config.server_url);
    preferences.putBool("configured", true);
    
    wifi_config.is_configured = true;
    
    Serial.println("✅ WiFi 설정 저장 완료");
    printWiFiConfig();
}

void startConfigPortal() {
    Serial.println("📱 WiFi 설정 포털 시작...");
    
    current_mode = CONFIG_MODE_AP;
    
    // AP 모드 시작
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("AP IP: %s\n", apIP.toString().c_str());
    Serial.printf("AP SSID: %s\n", AP_SSID);
    Serial.printf("AP Password: %s\n", AP_PASSWORD);
    
    // DNS 서버 시작 (Captive Portal)
    dns_server.start(53, "*", apIP);
    
    // 웹 서버 핸들러 설정
    config_server.on("/", handleRoot);
    config_server.on("/save", HTTP_POST, handleSave);
    config_server.on("/reset", handleReset);
    config_server.on("/status", handleStatus);
    config_server.onNotFound(handleRoot);  // Captive Portal
    
    config_server.begin();
    Serial.println("✅ 설정 포털이 시작되었습니다!");
    Serial.println("📱 스마트폰으로 WiFi 'T-Camera-Config'에 연결하고 브라우저를 열어주세요.");
}

void stopConfigPortal() {
    Serial.println("설정 포털 종료...");
    
    config_server.stop();
    dns_server.stop();
    WiFi.softAPdisconnect(true);
    
    current_mode = CONFIG_MODE_NORMAL;
}

void handleConfigPortal() {
    if (current_mode == CONFIG_MODE_AP) {
        dns_server.processNextRequest();
        config_server.handleClient();
        
        // 타임아웃 체크
        static unsigned long startTime = millis();
        if (millis() - startTime > CONFIG_TIMEOUT) {
            Serial.println("⏰ 설정 포털 타임아웃 - 기본 설정으로 진행");
            stopConfigPortal();
            // 기본 설정으로 폴백
            wifi_config.ssid = "PRO";
            wifi_config.password = "propro123";
            wifi_config.server_url = "http://192.168.0.27:3000/api/sensors";
            saveWiFiConfig();
        }
    }
}

bool connectWithConfig() {
    if (!wifi_config.is_configured) {
        Serial.println("WiFi 설정이 없습니다.");
        return false;
    }
    
    Serial.printf("WiFi 연결 시도: %s\n", wifi_config.ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    
    WiFi.begin(wifi_config.ssid.c_str(), wifi_config.password.c_str());
    
    unsigned long startTime = millis();
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            Serial.printf(" [%d]", attempts);
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi 연결 성공!");
        Serial.printf("IP 주소: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("신호 강도: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        Serial.println("❌ WiFi 연결 실패!");
        Serial.printf("상태 코드: %d\n", WiFi.status());
        
        // 연결 실패 시 설정 포털 시작
        Serial.println("설정 포털을 시작합니다...");
        startConfigPortal();
        return false;
    }
}

void startSmartConfig() {
    Serial.println("📱 SmartConfig 시작...");
    Serial.println("ESP-Touch 앱을 사용하여 WiFi 설정을 전송하세요.");
    
    current_mode = CONFIG_MODE_SMARTCONFIG;
    
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
}

void handleSmartConfig() {
    if (current_mode == CONFIG_MODE_SMARTCONFIG) {
        if (WiFi.smartConfigDone()) {
            Serial.println("✅ SmartConfig 완료!");
            
            wifi_config.ssid = WiFi.SSID();
            wifi_config.password = WiFi.psk();
            wifi_config.server_url = "http://192.168.0.27:3000/api/sensors"; // 기본값
            
            saveWiFiConfig();
            current_mode = CONFIG_MODE_NORMAL;
            
            Serial.printf("연결된 WiFi: %s\n", wifi_config.ssid.c_str());
        }
    }
}

void checkConfigButton() {
    // GPIO0 버튼을 5초간 누르면 설정 모드 진입
    static unsigned long pressStart = 0;
    static bool buttonPressed = false;
    
    if (digitalRead(0) == LOW) {  // 버튼 눌림
        if (!buttonPressed) {
            pressStart = millis();
            buttonPressed = true;
        } else if (millis() - pressStart > 5000) {
            Serial.println("🔄 설정 리셋 - 설정 포털 시작");
            preferences.clear();
            wifi_config.is_configured = false;
            startConfigPortal();
            buttonPressed = false;
        }
    } else {
        buttonPressed = false;
    }
}

void printWiFiConfig() {
    Serial.println("📋 현재 WiFi 설정:");
    Serial.printf("  SSID: %s\n", wifi_config.ssid.c_str());
    Serial.printf("  Password: %s\n", wifi_config.password.length() > 0 ? "***설정됨***" : "없음");
    Serial.printf("  Server URL: %s\n", wifi_config.server_url.c_str());
    Serial.printf("  구성 상태: %s\n", wifi_config.is_configured ? "완료" : "미완료");
}

// 웹 핸들러 함수들
void handleRoot() {
    String html = R"(
<!DOCTYPE html>
<html lang='ko'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>T-Camera WiFi 설정</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f8ff; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; margin-bottom: 30px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }
        input[type='text'], input[type='password'], input[type='url'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }
        .btn { background: #007bff; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; width: 100%; font-size: 16px; }
        .btn:hover { background: #0056b3; }
        .btn-danger { background: #dc3545; margin-top: 10px; }
        .btn-danger:hover { background: #c82333; }
        .info { background: #e7f3ff; padding: 15px; border-radius: 5px; margin-bottom: 20px; border-left: 4px solid #007bff; }
        .network-list { margin-bottom: 20px; }
        .network-item { padding: 8px; border: 1px solid #ddd; margin-bottom: 5px; border-radius: 3px; cursor: pointer; }
        .network-item:hover { background: #f8f9fa; }
    </style>
    <script>
        function selectNetwork(ssid) {
            document.getElementById('ssid').value = ssid;
        }
        function scanNetworks() {
            fetch('/scan').then(r => r.text()).then(data => {
                document.getElementById('networks').innerHTML = data;
            });
        }
    </script>
</head>
<body>
    <div class='container'>
        <div class='header'>
            <h1>📡 T-Camera WiFi 설정</h1>
            <p>센서 디바이스의 WiFi 설정을 구성하세요</p>
        </div>
        
        <div class='info'>
            <strong>💡 설정 방법:</strong><br>
            1. 연결할 WiFi 네트워크를 선택하거나 직접 입력<br>
            2. 비밀번호 입력<br>
            3. 서버 URL 확인 후 저장
        </div>

        <form action='/save' method='post'>
            <div class='form-group'>
                <label for='ssid'>WiFi 네트워크 이름 (SSID):</label>
                <input type='text' id='ssid' name='ssid' required placeholder='WiFi 이름을 입력하세요'>
            </div>
            
            <div class='form-group'>
                <label for='password'>WiFi 비밀번호:</label>
                <input type='password' id='password' name='password' placeholder='비밀번호를 입력하세요'>
            </div>
            
            <div class='form-group'>
                <label for='server_url'>서버 URL:</label>
                <input type='url' id='server_url' name='server_url' value='http://192.168.0.27:3000/api/sensors' required>
            </div>
            
            <button type='submit' class='btn'>💾 설정 저장</button>
        </form>
        
        <button onclick='location.href="/reset"' class='btn btn-danger'>🔄 설정 초기화</button>
    </div>
</body>
</html>
    )";
    
    config_server.send(200, "text/html", html);
}

void handleSave() {
    wifi_config.ssid = config_server.arg("ssid");
    wifi_config.password = config_server.arg("password");
    wifi_config.server_url = config_server.arg("server_url");
    
    Serial.println("📝 새 WiFi 설정 수신:");
    printWiFiConfig();
    
    saveWiFiConfig();
    
    String html = R"(
<!DOCTYPE html>
<html lang='ko'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>설정 완료</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f8ff; text-align: center; }
        .success { background: #d4edda; color: #155724; padding: 20px; border-radius: 10px; margin: 20px auto; max-width: 500px; border: 1px solid #c3e6cb; }
        .btn { background: #28a745; color: white; padding: 10px 20px; border: none; border-radius: 5px; text-decoration: none; display: inline-block; margin-top: 10px; }
    </style>
    <script>
        setTimeout(function() { window.location.href = '/status'; }, 3000);
    </script>
</head>
<body>
    <div class='success'>
        <h2>✅ 설정이 저장되었습니다!</h2>
        <p>디바이스가 새로운 WiFi로 연결을 시도합니다.</p>
        <p>3초 후 상태 페이지로 이동합니다...</p>
        <a href='/status' class='btn'>상태 확인</a>
    </div>
</body>
</html>
    )";
    
    config_server.send(200, "text/html", html);
    
    // 2초 후 설정 포털 종료하고 WiFi 연결 시도
    delay(2000);
    stopConfigPortal();
    connectWithConfig();
}

void handleReset() {
    Serial.println("🔄 WiFi 설정 초기화");
    
    preferences.clear();
    wifi_config.is_configured = false;
    wifi_config.ssid = "";
    wifi_config.password = "";
    wifi_config.server_url = "http://192.168.0.27:3000/api/sensors";
    
    String html = R"(
<!DOCTYPE html>
<html lang='ko'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>설정 초기화</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f8ff; text-align: center; }
        .warning { background: #fff3cd; color: #856404; padding: 20px; border-radius: 10px; margin: 20px auto; max-width: 500px; border: 1px solid #ffeaa7; }
        .btn { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; text-decoration: none; display: inline-block; margin-top: 10px; }
    </style>
    <script>
        setTimeout(function() { window.location.href = '/'; }, 3000);
    </script>
</head>
<body>
    <div class='warning'>
        <h2>🔄 설정이 초기화되었습니다!</h2>
        <p>모든 WiFi 설정이 삭제되었습니다.</p>
        <p>3초 후 설정 페이지로 이동합니다...</p>
        <a href='/' class='btn'>설정 페이지로</a>
    </div>
</body>
</html>
    )";
    
    config_server.send(200, "text/html", html);
}

void handleStatus() {
    String status = "연결 안됨";
    String ip = "없음";
    
    if (WiFi.status() == WL_CONNECTED) {
        status = "연결됨";
        ip = WiFi.localIP().toString();
    }
    
    String html = "<!DOCTYPE html><html lang='ko'><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>T-Camera 상태</title></head><body>";
    html += "<h1>📊 T-Camera 상태</h1>";
    html += "<p><strong>WiFi 상태:</strong> " + status + "</p>";
    html += "<p><strong>IP 주소:</strong> " + ip + "</p>";
    html += "<p><strong>SSID:</strong> " + wifi_config.ssid + "</p>";
    html += "<p><strong>서버 URL:</strong> " + wifi_config.server_url + "</p>";
    html += "<a href='/'>설정 페이지로</a>";
    html += "</body></html>";
    
    config_server.send(200, "text/html", html);
}