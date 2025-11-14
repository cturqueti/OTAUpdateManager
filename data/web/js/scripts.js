// scripts.js - Common JavaScript functions for ESP32 OTA

// Theme management
function toggleTheme() {
    const body = document.body;
    const themeToggles = document.querySelectorAll('.theme-toggle');
    
    if (body.classList.contains('dark')) {
        body.classList.remove('dark');
        body.classList.add('light');
        themeToggles.forEach(toggle => toggle.textContent = '🌙');
        localStorage.setItem('theme', 'light');
    } else {
        body.classList.remove('light');
        body.classList.add('dark');
        themeToggles.forEach(toggle => toggle.textContent = '☀️');
        localStorage.setItem('theme', 'dark');
    }
}

// Apply saved theme on load
function applySavedTheme() {
    const savedTheme = localStorage.getItem('theme') || 'dark';
    const body = document.body;
    const themeToggles = document.querySelectorAll('.theme-toggle');
    
    body.classList.remove('dark', 'light');
    body.classList.add(savedTheme);
    
    themeToggles.forEach(toggle => {
        toggle.textContent = savedTheme === 'dark' ? '☀️' : '🌙';
    });
}

// System information functions
function formatBytes(bytes) {
    if (!bytes || bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatRSSI(rssi) {
    if (!rssi) return 'N/A';
    if (rssi >= -50) return `${rssi} dBm (Excelente)`;
    if (rssi >= -60) return `${rssi} dBm (Bom)`;
    if (rssi >= -70) return `${rssi} dBm (Regular)`;
    return `${rssi} dBm (Fraco)`;
}

function getMonthNumber(monthStr) {
    const months = {
        'Jan': 0, 'Fev': 1, 'Mar': 2, 'Abr': 3, 'Mai': 4, 'Jun': 5,
        'Jul': 6, 'Ago': 7, 'Set': 8, 'Out': 9, 'Nov': 10, 'Dez': 11
    };
    return months[monthStr] || 0;
}

function getMonthName(month) {
    const months = ['Jan', 'Fev', 'Mar', 'Abr', 'Mai', 'Jun', 
                  'Jul', 'Ago', 'Set', 'Out', 'Nov', 'Dez'];
    return months[month];
}

// Modal functions
function showModal(modalId) {
    document.getElementById(modalId).style.display = 'block';
}

function closeModal(modalId) {
    document.getElementById(modalId).style.display = 'none';
}

// Status message functions
function showStatus(message, type) {
    const statusEl = document.getElementById('statusMessage');
    if (!statusEl) return;
    
    statusEl.textContent = message;
    statusEl.className = `status-message ${type}`;
    statusEl.style.display = 'block';
    
    setTimeout(() => {
        statusEl.style.display = 'none';
    }, 3000);
}

// OTA Update functions
async function checkForUpdates() {
    const otaStatus = document.getElementById('otaStatus');
    if (!otaStatus) {
        console.error('Elemento otaStatus não encontrado');
        return;
    }
    
    otaStatus.innerHTML = `
        <div style="text-align: center; color: var(--text-secondary); font-size: 0.9rem; margin: 0.5rem 0;">
            🔍 Verificando atualizações no servidor...
        </div>
    `;
    
    try {
        const response = await fetch('/check-updates');
        
        // Verifica se a resposta é OK
        if (!response.ok) {
            throw new Error(`Erro HTTP: ${response.status}`);
        }
        
        // Verifica se o content-type é JSON
        const contentType = response.headers.get('content-type');
        if (!contentType || !contentType.includes('application/json')) {
            throw new Error('Resposta não é JSON');
        }
        
        const data = await response.json();
        console.log('Resposta do servidor:', data);
        
        // Processa a resposta baseada no status
        if (data.status === 'update_available') {
            otaStatus.innerHTML = `
                <div style="background: var(--warning); color: white; padding: 0.5rem; border-radius: 0.375rem; margin: 0.25rem 0; text-align: center;">
                    <p style="margin: 0; font-weight: bold; font-size: 0.85rem;">🔄 ${data.message}</p>
                    <p style="margin: 0.25rem 0; font-size: 0.8rem;">Versão disponível: ${data.latest_version}</p>
                </div>
                <div style="text-align: center; margin: 0.5rem 0;">
                    <button class="btn" style="background: var(--accent-primary); padding: 0.4rem 0.8rem; font-size: 0.8rem;" onclick="performPullUpdate()">
                        📥 Instalar Atualização
                    </button>
                    <button class="btn" style="background: var(--secondary); padding: 0.4rem 0.8rem; font-size: 0.8rem;" onclick="checkForUpdates()">
                        🔍 Verificar Novamente
                    </button>
                </div>
            `;
        } else if (data.status === 'up_to_date') {
            otaStatus.innerHTML = `
                <div style="background: var(--success); color: white; padding: 0.5rem; border-radius: 0.375rem; margin: 0.25rem 0; text-align: center;">
                    <p style="margin: 0; font-weight: bold; font-size: 0.85rem;">✅ ${data.message}</p>
                    <p style="margin: 0.25rem 0; font-size: 0.8rem;">Versão atual: ${data.current_version}</p>
                </div>
                <div style="text-align: center; margin: 0.5rem 0;">
                    <button class="btn" style="background: var(--accent-primary); padding: 0.4rem 0.8rem; font-size: 0.8rem;" onclick="checkForUpdates()">
                        🔍 Verificar Novamente
                    </button>
                </div>
            `;
        } else {
            throw new Error(data.message || 'Status desconhecido na resposta');
        }
        
    } catch (error) {
        console.error('Erro ao verificar atualizações:', error);
        otaStatus.innerHTML = `
            <div style="background: var(--error); color: white; padding: 0.5rem; border-radius: 0.375rem; margin: 0.25rem 0; text-align: center;">
                <p style="margin: 0; font-weight: bold; font-size: 0.85rem;">❌ Erro ao verificar atualizações</p>
                <p style="margin: 0.25rem 0; font-size: 0.8rem;">${error.message}</p>
            </div>
            <div style="text-align: center; margin: 0.5rem 0;">
                <button class="btn" style="background: var(--accent-primary); padding: 0.4rem 0.8rem; font-size: 0.8rem;" onclick="checkForUpdates()">
                    🔍 Tentar Novamente
                </button>
            </div>
        `;
    }
}

// Initialize common functionality
document.addEventListener('DOMContentLoaded', function() {
    applySavedTheme();
    
    // Close modal when clicking outside
    window.onclick = function(event) {
        const modals = document.getElementsByClassName('modal');
        for (let modal of modals) {
            if (event.target === modal) {
                modal.style.display = 'none';
            }
        }
    }
});