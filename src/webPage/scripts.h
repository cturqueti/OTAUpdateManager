#pragma once

const char *jsScript = R"rawliteral(
function checkForUpdates() {
    fetch('/check-updates')
        .then(response => response.json())
        .then(data => {
            alert(data.message);
            // Recarrega a página para atualizar o status
            setTimeout(() => location.reload(), 2000);
        })
        .catch(error => {
            alert('Erro ao verificar atualizações: ' + error);
        });
}

function performPullUpdate() {
    if (confirm('Deseja instalar a atualização agora? O ESP32 reiniciará automaticamente.')) {
        fetch('/perform-update')
            .then(response => response.json())
            .then(data => {
                alert(data.message);
                // Mostra mensagem de reinício
                setTimeout(() => {
                    alert('Reiniciando... A página será atualizada em instantes.');
                }, 3000);
            })
            .catch(error => {
                alert('Erro ao iniciar atualização: ' + error);
            });
    }
}
    
function toggleTheme() {
    const currentPath = window.location.pathname;
    const currentTheme = new URLSearchParams(window.location.search).get('theme') || 'dark'; // ✅ PADRÃO DARK
    const newTheme = currentTheme === 'light' ? 'dark' : 'light';
    window.location.href = currentPath + '?theme=' + newTheme;
}

document.addEventListener('DOMContentLoaded', function() {
    const savedTheme = localStorage.getItem('ota-theme');
    const currentTheme = new URLSearchParams(window.location.search).get('theme');
    
    if (currentTheme) {
        localStorage.setItem('ota-theme', currentTheme);
    } else if (savedTheme && !currentTheme) {
        window.location.href = window.location.pathname + '?theme=' + savedTheme;
    } else if (!savedTheme && !currentTheme) {
        // ✅ NOVO: Se não há tema salvo nem parâmetro, redireciona para dark
        window.location.href = window.location.pathname + '?theme=dark';
    }
});

// No arquivo filesystem.h, adicione estas funções JavaScript:

// File upload functionality
function updateFileName() {
    const fileInput = document.getElementById('fileInput');
    const fileName = document.getElementById('fileName');
    const uploadBtn = document.getElementById('uploadSubmitBtn');
    const fileDropText = document.getElementById('fileDropText');
    
    if (fileInput.files.length > 0) {
        const file = fileInput.files[0];
        fileName.textContent = `Arquivo selecionado: ${file.name} (${formatFileSize(file.size)})`;
        fileDropText.textContent = file.name;
        uploadBtn.disabled = false;
    } else {
        fileName.textContent = '';
        fileDropText.textContent = 'Clique para selecionar ou arraste arquivos aqui';
        uploadBtn.disabled = true;
    }
}

function formatFileSize(bytes) {
    if (bytes < 1024) return bytes + ' B';
    else if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
    else return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
}

// Drag and drop functionality
function initializeDragAndDrop() {
    const dropArea = document.querySelector('.file-drop-area');
    
    ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
        dropArea.addEventListener(eventName, preventDefaults, false);
    });
    
    function preventDefaults(e) {
        e.preventDefault();
        e.stopPropagation();
    }
    
    ['dragenter', 'dragover'].forEach(eventName => {
        dropArea.addEventListener(eventName, highlight, false);
    });
    
    ['dragleave', 'drop'].forEach(eventName => {
        dropArea.addEventListener(eventName, unhighlight, false);
    });
    
    function highlight() {
        dropArea.classList.add('dragover');
    }
    
    function unhighlight() {
        dropArea.classList.remove('dragover');
    }
    
    dropArea.addEventListener('drop', handleDrop, false);
    
    function handleDrop(e) {
        const dt = e.dataTransfer;
        const files = dt.files;
        const fileInput = document.getElementById('fileInput');
        
        if (files.length > 0) {
            fileInput.files = files;
            updateFileName();
        }
    }
}

// Initialize when page loads
document.addEventListener('DOMContentLoaded', function() {
    initializeDragAndDrop();
    
    // Reset file input when modal opens
    document.getElementById('uploadModal').addEventListener('show', function() {
        document.getElementById('fileInput').value = '';
        updateFileName();
    });
});

function downloadFile(filePath) {
    if (!filePath) {
        showNotification('Caminho do arquivo inválido', 'error');
        return;
    }

    showNotification('Iniciando download...', 'success');
    
    // ✅ Método mais confiável: criar link temporário
    const downloadLink = document.createElement('a');
    downloadLink.href = '/filesystem-download?path=' + encodeURIComponent(filePath);
    downloadLink.download = filePath.split('/').pop(); // Nome do arquivo
    downloadLink.style.display = 'none';
    
    // ✅ Adicionar ao DOM e clicar
    document.body.appendChild(downloadLink);
    downloadLink.click();
    
    // ✅ Limpar após o click
    setTimeout(() => {
        document.body.removeChild(downloadLink);
        showNotification('Download do arquivo iniciado', 'success');
    }, 1000);
    
    console.log('Download iniciado:', filePath);
}

function downloadFileAlternative(filePath) {
    if (!filePath) return;
    
    showNotification('Abrindo download...', 'success');
    
    // ✅ Abrir em nova janela (fallback)
    const newWindow = window.open('/filesystem-download?path=' + encodeURIComponent(filePath), '_blank');
    
    // ✅ Fechar a janela após um tempo se ainda estiver aberta
    setTimeout(() => {
        if (newWindow && !newWindow.closed) {
            newWindow.close();
        }
    }, 5000);
}

// ✅ MELHORIA: Função para visualizar arquivos de texto (opcional)
function viewTextFile(filePath) {
    if (!filePath) return;
    
    // Para arquivos de texto pequenos, podemos abrir em uma nova janela
    if (filePath.endsWith('.txt') || filePath.endsWith('.log') || 
        filePath.endsWith('.json') || filePath.endsWith('.html') || 
        filePath.endsWith('.css') || filePath.endsWith('.js')) {
        
        window.open('/filesystem-download?path=' + encodeURIComponent(filePath), '_blank');
    } else {
        // Para outros tipos de arquivo, apenas faz download
        downloadFile(filePath);
    }
}

function validateUploadPath(input) {
    let path = input.value.trim();
    
    // Garante que comece com /
    if (!path.startsWith('/')) {
        path = '/' + path;
    }
    
    // Remove barras duplicadas no final
    if (path.endsWith('/') && path !== '/') {
        path = path.slice(0, -1);
    }
    
    input.value = path;
}

)rawliteral";