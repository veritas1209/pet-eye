#ifndef WEB_PAGES_H
#define WEB_PAGES_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>PetEye Configuration</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 500px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .status {
            background: #f0f0f0;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        .status-item {
            display: flex;
            justify-content: space-between;
            padding: 5px 0;
        }
        .online { color: #4CAF50; font-weight: bold; }
        .offline { color: #f44336; font-weight: bold; }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 16px;
            box-sizing: border-box;
            margin-top: 5px;
        }
        button {
            width: 100%;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 14px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            margin-top: 10px;
        }
        .scan-results {
            max-height: 150px;
            overflow-y: auto;
            border: 1px solid #ddd;
            border-radius: 8px;
            margin-top: 10px;
        }
        .network-item {
            padding: 10px;
            border-bottom: 1px solid #eee;
            cursor: pointer;
        }
        .network-item:hover { background: #f5f5f5; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🐾 PetEye Configuration</h1>
        
        <div class="status">
            <h3>System Status</h3>
            <div class="status-item">
                <span>Device ID:</span>
                <span>%DEVICE_ID%</span>
            </div>
            <div class="status-item">
                <span>WiFi Status:</span>
                <span class="%WIFI_CLASS%">%WIFI_STATUS%</span>
            </div>
            <div class="status-item">
                <span>IP Address:</span>
                <span>%IP_ADDRESS%</span>
            </div>
            <div class="status-item">
                <span>Camera:</span>
                <span class="%CAM_CLASS%">%CAM_STATUS%</span>
            </div>
            <div class="status-item">
                <span>Temperature:</span>
                <span>%TEMPERATURE%</span>
            </div>
        </div>
        
        <form action="/save" method="POST">
            <label for="ssid">WiFi Network Name (SSID)</label>
            <input type="text" id="ssid" name="ssid" required>
            <button type="button" onclick="scanNetworks()" style="background: #6c757d;">📡 Scan Networks</button>
            <div id="scanResults" class="scan-results" style="display: none;"></div>
            
            <label for="password" style="display: block; margin-top: 15px;">WiFi Password</label>
            <input type="password" id="password" name="password" required>
            
            <button type="submit">💾 Save Configuration</button>
        </form>
        
        <button onclick="window.location.href='/debug'">🔧 Debug Console</button>
    </div>
    
    <script>
        function scanNetworks() {
            const resultsDiv = document.getElementById('scanResults');
            resultsDiv.innerHTML = '<div style="padding: 10px;">Scanning...</div>';
            resultsDiv.style.display = 'block';
            
            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    resultsDiv.innerHTML = '';
                    data.networks.forEach(network => {
                        const item = document.createElement('div');
                        item.className = 'network-item';
                        item.innerHTML = `<strong>${network.ssid}</strong> (${network.rssi} dBm)`;
                        item.onclick = () => {
                            document.getElementById('ssid').value = network.ssid;
                            resultsDiv.style.display = 'none';
                        };
                        resultsDiv.appendChild(item);
                    });
                });
        }
    </script>
</body>
</html>
)rawliteral";

const char DEBUG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>PetEye Debug Console</title>
    <style>
        body {
            font-family: 'Courier New', monospace;
            background: #1e1e1e;
            color: #0f0;
            margin: 0;
            padding: 20px;
        }
        .console {
            background: #000;
            padding: 20px;
            border-radius: 10px;
            border: 2px solid #0f0;
            min-height: 400px;
            white-space: pre-wrap;
            overflow-y: auto;
            max-height: 60vh;
        }
        button {
            background: #0f0;
            color: #000;
            border: none;
            padding: 10px 20px;
            margin: 5px;
            cursor: pointer;
            border-radius: 5px;
            font-weight: bold;
        }
        .info {
            background: #111;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            border: 1px solid #0f0;
        }
    </style>
</head>
<body>
    <a href="/" style="color: #0f0;">← Back</a>
    <h1 style="color: #0f0;">PetEye Debug Console</h1>
    
    <div class="info">
        <strong>System Info:</strong><br>
        Free Heap: <span id="freeHeap">-</span> bytes<br>
        Uptime: <span id="uptime">-</span> seconds<br>
        WiFi RSSI: <span id="rssi">-</span> dBm<br>
        Temperature: <span id="temp">-</span>°C
    </div>
    
    <div class="console" id="console">Loading...</div>
    
    <div style="margin-top: 20px;">
        <button onclick="clearConsole()">Clear</button>
        <button onclick="testCamera()">Test Camera</button>
        <button onclick="testTemp()">Test Temp</button>
        <button onclick="testAPI()">Test API</button>
        <button onclick="reboot()">Reboot</button>
    </div>
    
    <script>
        function updateConsole() {
            fetch('/api/debug').then(r => r.text()).then(data => {
                document.getElementById('console').textContent = data;
            });
            
            fetch('/api/status').then(r => r.json()).then(data => {
                document.getElementById('freeHeap').textContent = data.freeHeap;
                document.getElementById('uptime').textContent = data.uptime;
                document.getElementById('rssi').textContent = data.rssi;
                document.getElementById('temp').textContent = data.temperature;
            });
        }
        
        function clearConsole() { fetch('/api/clear', {method: 'POST'}); }
        function testCamera() { fetch('/api/test/camera', {method: 'POST'}); }
        function testTemp() { fetch('/api/test/temperature', {method: 'POST'}); }
        function testAPI() { fetch('/api/test/api', {method: 'POST'}); }
        function reboot() {
            if(confirm('Reboot device?')) {
                fetch('/api/reboot', {method: 'POST'});
            }
        }
        
        setInterval(updateConsole, 2000);
        updateConsole();
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGES_H