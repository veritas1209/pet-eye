// src/wifi_portal.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// 설정 포털을 위한 전역 변수
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

const char* AP_SSID = "PetEye-Setup";
const char* AP_PASS = "12345678";

// HTML 페이지
const char* setupHTML = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>PetEye WiFi 설정</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 400px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .emoji {
            font-size: 48px;
            text-align: center;
            margin-bottom: 20px;
        }
        input[type='text'], input[type='password'] {
            width: 100%;
            padding: 12px;
            margin: 8px 0;
            border: 1px solid #ddd;
            border-radius: 5px;
            box-sizing: border-box;
            font-size: 16px;
        }
        label {
            color: #555;
            font-weight: bold;
            display: block;
            margin-top: 10px;
        }
        button {
            width: 100%;
            padding: 12px;
            margin-top: 20px;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
            transition: background 0.3s;
        }
        button:hover {
            background: #5a5fcf;
        }
        .scan-btn {
            background: #48bb78;
            margin-bottom: 20px;
        }
        .scan-btn:hover {
            background: #38a169;
        }
        .network-list {
            max-height: 200px;
            overflow-y: auto;
            border: 1px solid #ddd;
            border-radius: 5px;
            margin: 10px 0;
        }
        .network-item {
            padding: 10px;
            cursor: pointer;
            border-bottom: 1px solid #eee;
        }
        .network-item:hover {
            background: #f7f7f7;
        }
        .signal {
            float: right;
            color: #666;
        }
        .status {
            text-align: center;
            margin-top: 20px;
            padding: 10px;
            border-radius: 5px;
        }
        .success {
            background: #c6f6d5;
            color: #22543d;
        }
        .error {
            background: #fed7d7;
            color: #742a2a;
        }
    </style>
</head>
<body>
    <div class='container'>
        <div class='emoji'>🐾</div>
        <h1>PetEye WiFi 설정</h1>
        
        <button class='scan-btn' onclick='scanNetworks()'>📡 WiFi 네트워크 검색</button>
        
        <div id='networks' class='network-list' style='display:none;'></div>
        
        <form action='/save' method='post'>
            <label for='ssid'>WiFi 이름 (SSID)</label>
            <input type='text' id='ssid' name='ssid' required>
            
            <label for='pass'>비밀번호</label>
            <input type='password' id='pass' name='pass' required>
            
            <label for='server'>서버 주소 (선택)</label>
            <input type='text' id='server' name='server' placeholder='192.168.0.100:3000'>
            
            <button type='submit'>💾 저장하고 연결</button>
        </form>
        
        <div id='status'></div>
    </div>
    
    <script>
        function scanNetworks() {
            document.getElementById('status').innerHTML = '검색 중...';
            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    let html = '';
                    data.forEach(net => {
                        html += `<div class='network-item' onclick='selectNetwork("${net.ssid}")'>
                                   ${net.ssid} <span class='signal'>${net.rssi} dBm</span>
                                 </div>`;
                    });
                    document.getElementById('networks').innerHTML = html;
                    document.getElementById('networks').style.display = 'block';
                    document.getElementById('status').innerHTML = '';
                });
        }
        
        function selectNetwork(ssid) {
            document.getElementById('ssid').value = ssid;
            document.getElementById('networks').style.display = 'none';
        }
    </script>
</body>
</html>
)";

// WiFi 스캔 핸들러
void handleScan() {
    String json = "[";
    int n = WiFi.scanNetworks();
    
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i));
        json += "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
}

// 설정 저장 핸들러
void handleSave() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    String serverAddr = server.arg("server");
    
    // Preferences에 저장
    preferences.begin("peteye", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    if (serverAddr.length() > 0) {
        preferences.putString("server", serverAddr);
    }
    preferences.end();
    
    String html = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset='UTF-8'>
        <style>
            body { 
                font-family: Arial; 
                text-align: center; 
                padding: 50px;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            }
            .container {
                background: white;
                padding: 30px;
                border-radius: 10px;
                display: inline-block;
            }
            .emoji { font-size: 64px; }
        </style>
    </head>
    <body>
        <div class='container'>
            <div class='emoji'>✅</div>
            <h2>설정 저장 완료!</h2>
            <p>PetEye가 WiFi에 연결을 시도합니다...</p>
            <p>LED가 파란색으로 바뀌면 연결 성공입니다.</p>
        </div>
    </body>
    </html>
    )";
    
    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
}

// WiFi 설정 포털 시작
void startConfigPortal() {
    Serial.println("🌐 WiFi 설정 포털 시작");
    
    // AP 모드 시작
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP: ");
    Serial.println(IP);
    
    // DNS 서버 시작 (Captive Portal)
    dnsServer.start(53, "*", IP);
    
    // 웹 서버 핸들러 등록
    server.on("/", []() {
        server.send(200, "text/html", setupHTML);
    });
    server.on("/scan", handleScan);
    server.on("/save", handleSave);
    
    server.begin();
    Serial.println("웹 서버 시작됨");
}

// WiFi 연결 시도
bool connectToWiFi() {
    preferences.begin("peteye", true);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");
    preferences.end();
    
    if (ssid.length() == 0) {
        return false;
    }
    
    Serial.printf("WiFi 연결 시도: %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi 연결 성공!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    
    Serial.println("\n❌ WiFi 연결 실패");
    return false;
}

// 설정 포털 루프 처리
void handleConfigPortal() {
    dnsServer.processNextRequest();
    server.handleClient();
}