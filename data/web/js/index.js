// Index page specific JavaScript
let systemUptime = 0;
let uptimeInterval = null;
let clockInterval = null;
let lastServerTime = null;
let timeOffset = 0;

async function updateSystemInfo() {
    try {
        const response = await fetch('/api/system-info');
        const data = await response.json();
        
        console.log('Dados recebidos da API:', data);
        
        // Atualiza a interface com dados reais
        document.getElementById('ipAddress').textContent = data.wifi?.ip || window.location.hostname;
        document.getElementById('ssid').textContent = data.wifi?.ssid || 'WiFi-ESP32';
        document.getElementById('mdnsAddress').textContent = data.wifi?.mDNS || 'esp32s3.local';
        document.getElementById('firmwareVersion').textContent = data.system?.firmwareVersion || '1.0.0';
        document.getElementById('heapFree').textContent = formatBytes(data.hardware?.heapFree) || 'Calculando...';
        document.getElementById('cpuFreq').textContent = (data.hardware?.cpuFreq || '240') + ' MHz';
        document.getElementById('hostInfo').textContent = data.wifi?.mDNS || '.local';
        
        // Sincroniza o relógio com o servidor
        if (data.system?.currentTime) {
            lastServerTime = data.system.currentTime;
            syncClockWithServer();
        }
        
        // Atualiza uptime formatado
        if (data.system?.uptime) {
            systemUptime = data.system.uptime;
            updateUptimeDisplay();
        }
        
    } catch (error) {
        console.error('Erro ao carregar informações:', error);
        loadFallbackInfo();
    }
}

function updateUptimeDisplay() {
    const uptimeElement = document.getElementById('uptime');
    if (uptimeElement && systemUptime > 0) {
        systemUptime += 1000; // Incrementa 1 segundo
        
        const seconds = Math.floor(systemUptime / 1000);
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;
        
        let uptimeText = 'Uptime: ';
        if (days > 0) uptimeText += `${days}d `;
        if (hours > 0) uptimeText += `${hours}h `;
        if (minutes > 0) uptimeText += `${minutes}m `;
        uptimeText += `${secs}s`;
        
        uptimeElement.textContent = uptimeText;
    }
}

function syncClockWithServer() {
    if (!lastServerTime) return;
    
    // Parse do formato do ESP32: "07/Nov/2025 14:07:12"
    const timeParts = lastServerTime.split(' ');
    if (timeParts.length !== 2) return;
    
    const datePart = timeParts[0];
    const timePart = timeParts[1];
    
    const dateParts = datePart.split('/');
    if (dateParts.length !== 3) return;
    
    const day = parseInt(dateParts[0]);
    const month = getMonthNumber(dateParts[1]);
    const year = parseInt(dateParts[2]);
    
    const timePartsSplit = timePart.split(':');
    if (timePartsSplit.length !== 3) return;
    
    const hours = parseInt(timePartsSplit[0]);
    const minutes = parseInt(timePartsSplit[1]);
    const seconds = parseInt(timePartsSplit[2]);
    
    // Cria data do servidor
    const serverDate = new Date(year, month, day, hours, minutes, seconds);
    const clientDate = new Date();
    
    // Calcula a diferença entre servidor e cliente
    timeOffset = serverDate.getTime() - clientDate.getTime();
    
    // Atualiza o relógio imediatamente
    updateClockDisplay();
}

function updateClockDisplay() {
    const clockElement = document.getElementById('currentTime');
    if (!clockElement) return;
    
    let now;
    if (timeOffset !== 0) {
        // Usa o tempo sincronizado com o servidor
        now = new Date(Date.now() + timeOffset);
    } else {
        // Usa o tempo local como fallback
        now = new Date();
    }
    
    const day = now.getDate().toString().padStart(2, '0');
    const month = getMonthName(now.getMonth());
    const year = now.getFullYear();
    const hours = now.getHours().toString().padStart(2, '0');
    const minutes = now.getMinutes().toString().padStart(2, '0');
    const seconds = now.getSeconds().toString().padStart(2, '0');
    
    clockElement.textContent = `${day}/${month}/${year} ${hours}:${minutes}:${seconds}`;
}

function startClock() {
    // Atualiza o relógio a cada segundo
    updateClockDisplay();
    clockInterval = setInterval(updateClockDisplay, 1000);
}

function loadFallbackInfo() {
    document.getElementById('ipAddress').textContent = window.location.hostname;
    document.getElementById('ssid').textContent = 'WiFi-ESP32';
    document.getElementById('firmwareVersion').textContent = '1.0.0';
    document.getElementById('heapFree').textContent = 'Calculando...';
    document.getElementById('cpuFreq').textContent = '240 MHz';
    document.getElementById('hostInfo').textContent = '.local';
    
    // Inicia contador local se não conseguir dados da API
    if (systemUptime === 0) {
        systemUptime = 1000; // 1 segundo inicial
        updateUptimeDisplay();
    }
    
    // Garante que o relógio local continue funcionando
    if (!clockInterval) {
        startClock();
    }
}

// Inicialização
document.addEventListener('DOMContentLoaded', function() {
    // Inicia o relógio imediatamente
    startClock();
    
    // Carrega informações do sistema
    updateSystemInfo();

    // Atualiza informações
    let systemInfoTimeoutMinutes = 5;
    let systemInfoTimeoutSeconds = 0;
    setInterval(updateSystemInfo, systemInfoTimeoutMinutes * 60 * 1000 + systemInfoTimeoutSeconds * 1000);

    // Inicia o contador de uptime em tempo real
    uptimeInterval = setInterval(updateUptimeDisplay, 1000);
    
    // Verifica atualizações 
    let updatesTimeoutHours = 1;
    let updatesTimeoutMinutes = 0;
    let updatesTimeoutSeconds = 0;
    setTimeout(checkForUpdates, updatesTimeoutHours * 60 * 60 * 1000 + updatesTimeoutMinutes * 60 * 1000 + updatesTimeoutSeconds * 1000);

});

// Limpa os intervalos quando a página é fechada
window.addEventListener('beforeunload', function() {
    if (uptimeInterval) {
        clearInterval(uptimeInterval);
    }
    if (clockInterval) {
        clearInterval(clockInterval);
    }
});
