// config.js - JavaScript específico para a página de configurações

let currentSettings = {
    autoUpdate: false,
    serverUrl: "",
    updateInterval: 24,
    currentMode: 2
};

// Função para carregar as configurações atuais
async function loadSettings() {
    try {
        showLoadingState();
        
        const response = await fetch('/api/config');
        if (!response.ok) {
            throw new Error(`Erro HTTP: ${response.status}`);
        }
        
        const data = await response.json();
        console.log('📥 Configurações recebidas:', data);
        
        // Atualiza o objeto currentSettings com os dados recebidos
        currentSettings.autoUpdate = data.autoUpdate || false;
        currentSettings.serverUrl = data.serverUrl || "";
        currentSettings.updateInterval = data.updateInterval || 24;
        currentSettings.lastUpdateCheck = data.lastUpdateCheck || 'Nunca';
        currentSettings.currentMode = data.currentMode || 2;
        
        updateUI();
        showSuccess('Configurações carregadas com sucesso');
        
    } catch (error) {
        console.error('❌ Erro ao carregar configurações:', error);
        showError('Erro ao carregar configurações: ' + error.message);
    }
}

// Função para salvar as configurações
async function saveSettings() {
    try {
        showLoadingState();
        
        const settingsToSave = {
            autoUpdate: currentSettings.autoUpdate,
            serverUrl: currentSettings.serverUrl,
            updateInterval: currentSettings.updateInterval
        };
        
        console.log('📤 Enviando configurações:', settingsToSave);
        
        const response = await fetch('/api/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(settingsToSave)
        });
        
        if (!response.ok) {
            const errorText = await response.text();
            throw new Error(`Erro HTTP ${response.status}: ${errorText}`);
        }
        
        const result = await response.json();
        console.log('✅ Resposta do servidor:', result);
        
        showSuccess('Configurações salvas com sucesso');
        
        // Recarrega as configurações para garantir sincronização
        setTimeout(loadSettings, 500);
        
    } catch (error) {
        console.error('❌ Erro ao salvar configurações:', error);
        showError('Erro ao salvar configurações: ' + error.message);
    }
}

// Função para atualizar a UI com as configurações atuais
function updateUI() {
    // Atualiza checkbox
    const autoUpdateCheckbox = document.getElementById('autoUpdate');
    autoUpdateCheckbox.checked = currentSettings.autoUpdate;
    
    // Atualiza URL do servidor
    const serverUrlInput = document.getElementById('serverUrl');
    serverUrlInput.value = currentSettings.serverUrl;

    // Atualiza intervalo
    const updateIntervalSelect = document.getElementById('updateInterval');
    updateIntervalSelect.value = currentSettings.updateInterval;
    
    // CORREÇÃO: Mostrar campos quando autoUpdate estiver ATIVADO
    const serverUrlGroup = document.getElementById('serverUrlGroup');
    const intervalGroup = document.getElementById('updateIntervalGroup');
    
    serverUrlGroup.style.display = currentSettings.autoUpdate ? 'block' : 'none';
    intervalGroup.style.display = currentSettings.autoUpdate ? 'block' : 'none';
    
    // Atualiza status
    document.getElementById('currentAutoUpdateStatus').textContent = 
        currentSettings.autoUpdate ? '✅ Ativado' : '❌ Desativado';

    document.getElementById('currentServerUrl').textContent = 
        currentSettings.serverUrl || 'Não configurado';
    
    document.getElementById('currentUpdateInterval').textContent = 
        currentSettings.autoUpdate ? `${currentSettings.updateInterval} horas` : 'N/A';
    
    document.getElementById('lastUpdateCheck').textContent = 
        currentSettings.lastUpdateCheck || 'Nunca';
        
    // Atualiza informações do modo
    updateModeInfo();
}

// Função chamada quando o checkbox é alterado
function toggleAutoUpdate() {
    const autoUpdateCheckbox = document.getElementById('autoUpdate');
    currentSettings.autoUpdate = autoUpdateCheckbox.checked;
    
    const serverUrlGroup = document.getElementById('serverUrlGroup');
    const intervalGroup = document.getElementById('updateIntervalGroup');
    
    // CORREÇÃO: Mostrar campos quando autoUpdate estiver ATIVADO
    serverUrlGroup.style.display = currentSettings.autoUpdate ? 'block' : 'none';
    intervalGroup.style.display = currentSettings.autoUpdate ? 'block' : 'none';
}

// Função chamada quando o URL do servidor é alterado
function serverUrlChanged() {
    const serverUrlInput = document.getElementById('serverUrl');
    currentSettings.serverUrl = serverUrlInput.value.trim();
}

// Função chamada quando o intervalo é alterado
function updateIntervalChanged() {
    const updateIntervalSelect = document.getElementById('updateInterval');
    currentSettings.updateInterval = parseInt(updateIntervalSelect.value);
}

// Funções auxiliares para feedback
function showLoadingState() {
    // Poderia adicionar um spinner aqui
    console.log('Carregando...');
}

function showSuccess(message) {
    // Substitua por um sistema de notificação mais elegante se preferir
    const notification = document.createElement('div');
    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: var(--success);
        color: white;
        padding: 1rem;
        border-radius: 0.375rem;
        z-index: 1000;
        animation: slideIn 0.3s ease-out;
    `;
    notification.textContent = '✅ ' + message;
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.remove();
    }, 3000);
}

function showError(message) {
    const notification = document.createElement('div');
    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: var(--error);
        color: white;
        padding: 1rem;
        border-radius: 0.375rem;
        z-index: 1000;
        animation: slideIn 0.3s ease-out;
    `;
    notification.textContent = '❌ ' + message;
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.remove();
    }, 5000);
}

// Inicialização quando a página carrega
document.addEventListener('DOMContentLoaded', function() {
    console.log('🚀 Página de configurações inicializada');
    
    // Carrega as configurações quando a página abre
    loadSettings();
    
    // Atualiza informações do sistema
    updateSystemInfo();
    setInterval(updateSystemInfo, 30000); // Atualiza a cada 30 segundos
});

// Funções para atualizar tempo e uptime (similares às outras páginas)
function updateCurrentTime() {
    const now = new Date();
    document.getElementById('currentTime').textContent = 
        now.toLocaleString('pt-BR', {
            day: '2-digit',
            month: '2-digit',
            year: 'numeric',
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit'
        });
}

function updateUptime() {
    // Esta função precisaria ser implementada com dados do servidor
    // Similar à implementação em system.js
}

function updateModeInfo() {
    const modeNames = {
        0: 'Manual',
        1: 'Automático', 
        2: 'Híbrido'
    };
    
    const modeElement = document.getElementById('currentMode');
    if (modeElement) {
        modeElement.textContent = modeNames[currentSettings.currentMode] || 'Desconhecido';
    }
}

async function updateSystemInfo() {
    try {
        const response = await fetch('/api/uptime');
        if (response.ok) {
            const uptime = await response.text();
            document.getElementById('uptime').textContent = `Uptime: ${uptime}`;
        }
    } catch (error) {
        console.log('Não foi possível atualizar uptime');
    }
    
    // Atualiza tempo atual
    updateCurrentTime();
}
setInterval(updateCurrentTime, 1000);