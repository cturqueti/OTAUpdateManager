
// System page specific JavaScript
let systemUptime = 0;
let uptimeInterval = null;

// Função para buscar informações do sistema via API
async function fetchSystemInfo() {
    try {
        const response = await fetch('/api/system-info');
        if (!response.ok) {
            throw new Error('Erro ao buscar informações do sistema');
        }
        return await response.json();
    } catch (error) {
        console.error('Erro:', error);
        throw error;
    }
}

// Função para atualizar o tempo de atividade em tempo real
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
        let uptimeNumber = "";
        if (days > 0) uptimeNumber += `${days}d `;
        if (hours > 0) uptimeNumber += `${hours}h `;
        if (minutes > 0) uptimeNumber += `${minutes}m `;
        uptimeNumber += `${secs}s`;
        document.getElementById('sys-uptime').textContent = uptimeNumber;
        uptimeText += uptimeNumber;
        
        uptimeElement.textContent = uptimeText;
    }
}

// Função principal para atualizar todas as informações do sistema
async function updateSystemInfo() {
    try {
        const data = await fetchSystemInfo();
        console.log('Dados recebidos da API:', data);
        
        // Atualiza informações do WiFi
        if (data.wifi) {
            document.getElementById('wifi-ip').textContent = data.wifi.ip || 'N/A';
            document.getElementById('wifi-ssid').textContent = data.wifi.ssid || 'N/A';
            document.getElementById('wifi-rssi').textContent = formatRSSI(data.wifi.rssi);
            document.getElementById('wifi-mac').textContent = data.wifi.mac || 'N/A';
            document.getElementById('wifi-hostname').textContent = data.wifi.hostname || 'N/A';
            document.getElementById('wifi-gateway').textContent = data.wifi.gateway || 'N/A';
            document.getElementById('wifi-subnet').textContent = data.wifi.subnet || 'N/A';
            document.getElementById('wifi-dns').textContent = data.wifi.dns || 'N/A';
            document.getElementById('wifi-mdns').textContent = data.wifi.mDNS || 'N/A';
        }
        
        // Atualiza informações de Hardware
        if (data.hardware) {
            document.getElementById('hw-chipmodel').textContent = data.hardware.chipModel || 'N/A';
            document.getElementById('hw-cores').textContent = data.hardware.chipCores || 'N/A';
            document.getElementById('hw-revision').textContent = data.hardware.chipRevision || 'N/A';
            document.getElementById('hw-cpufreq').textContent = (data.hardware.cpuFreq || 'N/A') + ' MHz';
            document.getElementById('hw-heapfree').textContent = formatBytes(data.hardware.heapFree);
            document.getElementById('hw-heaptotal').textContent = formatBytes(data.hardware.heapTotal);
            document.getElementById('hw-heapmin').textContent = formatBytes(data.hardware.heapMin);
            document.getElementById('hw-psram').textContent = data.hardware.psramSize > 0 ? formatBytes(data.hardware.psramSize) : 'Não disponível';
            document.getElementById('hw-flashsize').textContent = formatBytes(data.hardware.flashSize);
            document.getElementById('hw-flashspeed').textContent = data.hardware.flashSpeed ? (data.hardware.flashSpeed / 1000000) + ' MHz' : 'N/A';
            document.getElementById('hw-sdkversion').textContent = data.hardware.sdkVersion || 'N/A';
        }
        
        // Atualiza informações do Sistema
        if (data.system) {
            document.getElementById('sys-version').textContent = data.system.firmwareVersion || 'N/A';
            document.getElementById('sys-currenttime').textContent = data.system.currentTime || 'N/A';
            document.getElementById('sys-resetreason').textContent = data.system.resetReason || 'N/A';
            document.getElementById('sys-resetinfo').textContent = data.system.resetInfo || 'N/A';
            
            // Atualiza uptime
            if (data.system.uptime) {
                systemUptime = data.system.uptime;
                updateUptimeDisplay();
            }
        }
        
        // Atualiza informações do Sistema de Arquivos
        if (data.filesystem) {
            document.getElementById('fs-total').textContent = formatBytes(data.filesystem.totalBytes);
            document.getElementById('fs-used').textContent = formatBytes(data.filesystem.usedBytes);
            document.getElementById('fs-free').textContent = formatBytes(data.filesystem.freeBytes);
            
            const usagePercent = data.filesystem.totalBytes > 0 ? 
                ((data.filesystem.usedBytes / data.filesystem.totalBytes) * 100).toFixed(1) : 0;
            document.getElementById('fs-usage').textContent = usagePercent + '%';
            document.getElementById('fs-progress-bar').style.width = usagePercent + '%';
        }
        
        // Atualiza footer
        document.getElementById('footer-hostinfo').textContent = data.wifi?.hostname || 'ESP32';
        
    } catch (error) {
        console.error('Erro ao carregar informações:', error);
        // Mostra mensagem de erro em elementos principais
        document.getElementById('current-time').textContent = 'Erro ao carregar';
        document.getElementById('uptime').textContent = 'Uptime: Erro ao carregar';
    }
}

// Funções placeholder para OTA
function performPullUpdate() {
    alert('Funcionalidade de atualização OTA Pull será implementada aqui');
}

// Inicialização quando a página carrega
document.addEventListener('DOMContentLoaded', function() {
    // Carrega informações imediatamente
    updateSystemInfo();
    
    // Atualiza informações
    let systemInfoTimeoutMinutes = 1;
    let systemInfoTimeoutSeconds = 5;
    setInterval(updateSystemInfo, systemInfoTimeoutMinutes * 60 * 1000 + systemInfoTimeoutSeconds * 1000);

    
    // Inicia o contador de uptime em tempo real
    uptimeInterval = setInterval(updateUptimeDisplay, 1000);
    
    // Verifica atualizações 
    let updatesTimeoutHours = 0;
    let updatesTimeoutMinutes = 1;
    let updatesTimeoutSeconds = 0;
    setTimeout(checkForUpdates, updatesTimeoutHours * 60 * 60 * 1000 + updatesTimeoutMinutes * 60 * 1000 + updatesTimeoutSeconds * 1000);

});

// Limpa o intervalo quando a página é fechada
window.addEventListener('beforeunload', function() {
    if (uptimeInterval) {
        clearInterval(uptimeInterval);
    }
});
