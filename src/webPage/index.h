#pragma once

const char *htmlIndex = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{{TITLE}}</title>
    <link rel="icon" type="image/png" href="/assets/favicon.png">
    <link rel="stylesheet" href="/assets/styles.css">
    <script src="/assets/scripts.js"></script>
    <style>{{CSS}}</style>
</head>
<body class="{{THEME}}">
    <div class="container">
        <nav class="navbar">
            <a href="/" class="nav-brand">OTA Update</a>
            <div class="nav-links">
                <a href="/">Home</a>
                <a href="/update">Upload</a>
                <a href="/system">System</a>
                <button class="theme-toggle nav-toggle" onclick="toggleTheme()">
                    {{THEME_BUTTON}}
                </button>
            </div>
        </nav>
        
        <main class="main-content">
            <div class="header">
                <h1>ESP32 OTA Update</h1>
                <button class="theme-toggle" onclick="toggleTheme()">
                    {{THEME_BUTTON}}
                </button>
            </div>
            {{CONTENT}}
        </main>
        
        <footer class="footer">
            <p>ESP32 OTA Update - {{HOST_INFO}}</p>
        </footer>
    </div>
    <script>{{JS}}</script>
</body>
</html>
)rawliteral";