#pragma once

const char *htmlUpdate = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{{TITLE}}</title>
    <link rel="icon" type="image/png" href="/favicon.ico">
    <link rel="shortcut icon" type="image/png" href="/favicon.ico">
    <link rel="apple-touch-icon" type="image/png" href="/favicon.ico">
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
                <h1>Firmware Upload</h1>
            </div>
            
            <div class="upload-form">
                <form method='POST' action='/doUpdate' enctype='multipart/form-data' id='uploadForm'>
                    <div class="file-input" onclick="document.getElementById('firmwareFile').click()">
                        <p>Click to select firmware</p>
                        <p style="font-size: 0.9rem; color: var(--text-secondary);">File.bin</p>
                        <input type='file' name='update' id='firmwareFile' accept='.bin' required style="display: none;" onchange='updateFileName(this)'>
                    </div>
                    <div id='fileName' style="margin: 1rem 0; text-align: center;"></div>

                    <button type='submit' class='btn' id='submitBtn'>Start Upload</button>
                </form>

                <div class="progress" id="progress" style="display: none;">
                    <div class="progress-bar" id="progressBar"></div>
                </div>

                <div id="status" style="text-align: center; margin: 1rem 0;"></div>
            </div>
        </main>
        
        <footer class="footer">
            <p>ESP32 OTA Update - {{HOST_INFO}}</p>
        </footer>
    </div>
    <script>{{JS}}</script>
    <script>
        function updateFileName(input) {
            const fileNameDiv = document.getElementById('fileName');
            if (input.files.length > 0) {
                fileNameDiv.innerHTML = '<strong>Selected file:</strong> ' + input.files[0].name;
            } else {
                fileNameDiv.innerHTML = '';
            }
        }
        
        function validateFirmwareFile(file) {
            const fileName = file.name.toLowerCase();
            
            if (!fileName.endsWith('.bin')) {
                alert('❌ Por favor, selecione um arquivo .bin');
                return false;
            }
            
            // Verifica se o nome do arquivo contém padrões de versão
            const versionPattern = /[vV]?\d+\.\d+\.\d+/;
            if (versionPattern.test(fileName)) {
                const versionMatch = fileName.match(versionPattern)[0];
                alert('🔍 Versão detectada no nome: ' + versionMatch);
            }
            
            return true;
        }

        // ✅ ADICIONAR: Event listener para validação
        document.getElementById('firmwareFile').addEventListener('change', function(e) {
            if (e.target.files.length > 0) {
                if (!validateFirmwareFile(e.target.files[0])) {
                    e.target.value = ''; // Limpa seleção
                    document.getElementById('fileName').innerHTML = '';
                }
            }
        });

        document.getElementById('uploadForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const fileInput = document.getElementById('firmwareFile');
            const submitBtn = document.getElementById('submitBtn');
            const progress = document.getElementById('progress');
            const progressBar = document.getElementById('progressBar');
            const status = document.getElementById('status');
            
            if (!fileInput.files.length) {
                status.innerHTML = '<div style="color: var(--error);">Please select a .bin file</div>';
                return;
            }
            
            // ✅ ADICIONAR: Validação final no submit
            if (!validateFirmwareFile(fileInput.files[0])) {
                status.innerHTML = '<div style="color: var(--error);">Invalid firmware file</div>';
                return;
            }
            
            const formData = new FormData();
            formData.append('update', fileInput.files[0]);
            
            submitBtn.disabled = true;
            submitBtn.textContent = 'Uploading...';
            progress.style.display = 'block';
            status.innerHTML = '<div style="color: var(--accent-primary);">Starting upload...</div>';
            
            const xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percent = (e.loaded / e.total) * 100;
                    progressBar.style.width = percent + '%';
                    status.innerHTML = '<div style="color: var(--accent-primary);">Uploading: ' + percent.toFixed(1) + '%</div>';
                }
            });
            
            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    status.innerHTML = '<div style="color: var(--success);">Upload completed! Restarting...</div>';
                    progressBar.style.background = 'var(--success)';
                } else {
                    status.innerHTML = '<div style="color: var(--error);">Error: ' + xhr.responseText + '</div>';
                    submitBtn.disabled = false;
                    submitBtn.textContent = 'Try Again';
                }
            });
            
            xhr.open('POST', '/doUpdate');
            xhr.send(formData);
        });
    </script>
</body>
</html>
)rawliteral";