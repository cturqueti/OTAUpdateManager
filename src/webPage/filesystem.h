#pragma once

const char *htmlFilesystem = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR" data-theme="{{THEME}}">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{{TITLE}} - ESP32 File Manager</title>
    <style>{{CSS}}</style>
</head>
<body class="{{THEME}}">
    <div class="container">
        <nav class="navbar">
            <a href="/" class="nav-brand">📁 File Manager</a>
            <div class="nav-links">
                <a href="/">🏠 Home</a>
                <a href="/update">📤 Upload</a>
                <a href="/system">ℹ️ System</a>
                <a href="/toggle-theme?theme={{THEME}}&path={{CURRENT_PATH}}" class="theme-toggle">
                    {{THEME_BUTTON}}
                </a>
            </div>
        </nav>

        <main class="main-content">
            <div class="header">
                <div class="header-left">
                    <h1>📁 File Manager</h1>
                    <div class="time-info">
                        <span class="system-time">{{ESP32_TIME}}</span>
                        <span class="uptime">Uptime: {{UPTIME}}</span>
                    </div>
                </div>
                <div class="header-controls">
                    <span class="host-info">{{HOST_INFO}}</span>
                </div>
            </div>

            <!-- Breadcrumb Navigation -->
            <div class="breadcrumb">
                <a href="/filesystem?path=/">📁 Root</a>
                {{BREADCRUMB}}
            </div>

            <!-- File Operations -->
            <div class="file-actions">
                <button class="btn btn-primary" onclick="showCreateFolderModal('{{CURRENT_PATH}}')">
                    📁 Nova Pasta
                </button>
                <button class="btn btn-primary" onclick="showUploadModal('{{CURRENT_PATH}}')">
                    📤 Upload Arquivo
                </button>
                <!-- ✅ CORREÇÃO: Botão de atualizar mantém o tema -->
                <a href="/filesystem?path={{CURRENT_PATH}}&theme={{THEME}}" class="btn btn-secondary">
                    🔄 Atualizar
                </a>
            </div>

            <!-- File List -->
            <div class="file-list-container">
                <h3>Arquivos em: {{CURRENT_PATH}}</h3>
                {{FILE_LIST}}
            </div>
        </main>

        <footer class="footer">
            <p>ESP32 File Manager - {{HOST_INFO}}</p>
        </footer>

        <!-- Upload Modal -->
        <div id="uploadModal" class="modal">
            <div class="modal-content">
                <div class="modal-header">
                    <h3>📤 Upload de Arquivo</h3>
                    <span class="close" onclick="closeUploadModal()">&times;</span>
                </div>
                <div class="modal-body">
                    <form id="uploadForm" enctype="multipart/form-data">
                        <div class="form-group">
                            <label for="fileInput">Selecione o arquivo:</label>
                            <div class="file-drop-area" onclick="document.getElementById('fileInput').click()">
                                <div class="icon">📁</div>
                                <p id="fileDropText">Clique para selecionar ou arraste arquivos aqui</p>
                                <input type="file" id="fileInput" name="file" required 
                                    style="display: none;" onchange="updateFileName()">
                            </div>
                            <div id="fileName" style="margin-top: 0.5rem; font-size: 0.9rem; color: var(--text-secondary);"></div>
                        </div>
                        <div class="form-group">
                            <label for="uploadPath">Pasta de destino:</label>
                            <input type="text" id="uploadPath" name="uploadPath" 
                                placeholder="/caminho/da/pasta" value="{{CURRENT_PATH}}" 
                                onchange="validateUploadPath(this)">
                            <small style="color: var(--text-secondary); font-size: 0.8rem;">
                                Digite o caminho completo da pasta de destino (deve começar com /)
                            </small>
                        </div>
                        <div class="form-actions">
                            <button type="button" class="btn btn-secondary" onclick="closeUploadModal()">
                                Cancelar
                            </button>
                            <button type="submit" class="btn btn-primary" id="uploadSubmitBtn" disabled>
                                📤 Fazer Upload
                            </button>
                        </div>
                    </form>
                </div>
            </div>
        </div>

        <!-- Create Folder Modal -->
        <div id="createFolderModal" class="modal">
            <div class="modal-content">
                <div class="modal-header">
                    <h3>📁 Criar Nova Pasta</h3>
                    <span class="close" onclick="closeCreateFolderModal()">&times;</span>
                </div>
                <div class="modal-body">
                    <form id="createFolderForm">
                        <div class="form-group">
                            <label for="folderName">Nome da pasta:</label>
                            <input type="text" id="folderName" name="folderName" 
                                   placeholder="Nome da nova pasta" required>
                        </div>
                        <div class="form-group">
                            <label for="folderPath">Localização:</label>
                            <input type="text" id="folderPath" name="folderPath" placeholder="/caminho/da/pasta" value="{{CURRENT_PATH}}">
                            <small style="color: var(--text-secondary); font-size: 0.8rem;">
                                Digite o caminho completo onde a pasta será criada
                            </small>
                        </div>
                        <div class="form-actions">
                            <button type="button" class="btn btn-secondary" 
                                    onclick="closeCreateFolderModal()">
                                Cancelar
                            </button>
                            <button type="submit" class="btn btn-primary">
                                📁 Criar Pasta
                            </button>
                        </div>
                    </form>
                </div>
            </div>
        </div>

        <!-- Delete Confirmation Modal -->
        <div id="deleteModal" class="modal">
            <div class="modal-content">
                <div class="modal-header">
                    <h3>🗑️ Confirmar Exclusão</h3>
                    <span class="close" onclick="closeDeleteModal()">&times;</span>
                </div>
                <div class="modal-body">
                    <p id="deleteMessage">Tem certeza que deseja excluir este item?</p>
                    <div class="form-actions">
                        <button type="button" class="btn btn-secondary" 
                                onclick="closeDeleteModal()">
                            Cancelar
                        </button>
                        <button type="button" class="btn btn-danger" 
                                onclick="confirmDelete()">
                            🗑️ Excluir
                        </button>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script>{{JS}}</script>
    <script>
        let currentDeletePath = '';
        let currentDeleteIsDir = false;
        let currentDirectoryPath = '{{CURRENT_PATH}}';

        function showUploadModal(path) {
            const uploadPathInput = document.getElementById('uploadPath');
            uploadPathInput.value = path || currentDirectoryPath;
            
            setTimeout(() => {
                uploadPathInput.focus();
                uploadPathInput.select();
            }, 100);
            
            document.getElementById('uploadModal').style.display = 'block';
        }

        function closeUploadModal() {
            document.getElementById('uploadModal').style.display = 'none';
            document.getElementById('uploadForm').reset();
            document.getElementById('fileName').textContent = '';
            document.getElementById('fileDropText').textContent = 'Clique para selecionar ou arraste arquivos aqui';
            document.getElementById('uploadSubmitBtn').disabled = true;
            document.getElementById('uploadPath').value = currentDirectoryPath;
        }

        function showCreateFolderModal(path) {
            const folderPathInput = document.getElementById('folderPath');
            folderPathInput.value = path || currentDirectoryPath;
            
            setTimeout(() => {
                folderPathInput.focus();
                folderPathInput.select();
            }, 100);
            
            document.getElementById('createFolderModal').style.display = 'block';
            
            setTimeout(() => {
                document.getElementById('folderName').focus();
            }, 150);
        }

        function closeCreateFolderModal() {
            document.getElementById('createFolderModal').style.display = 'none';
            document.getElementById('createFolderForm').reset();
            document.getElementById('folderPath').value = currentDirectoryPath;
        }

        function showDeleteModal(path, isDir, name) {
            currentDeletePath = path;
            currentDeleteIsDir = isDir;
            const type = isDir ? 'pasta' : 'arquivo';
            document.getElementById('deleteMessage').textContent = 
                `Tem certeza que deseja excluir ${type} "${name}"?`;
            document.getElementById('deleteModal').style.display = 'block';
        }

        function closeDeleteModal() {
            document.getElementById('deleteModal').style.display = 'none';
            currentDeletePath = '';
            currentDeleteIsDir = false;
        }

        function confirmDelete() {
            if (currentDeletePath) {
                fetch('/filesystem-delete', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: `path=${encodeURIComponent(currentDeletePath)}&isDir=${currentDeleteIsDir}`
                })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'success') {
                        showNotification(data.message, 'success');
                        setTimeout(() => {
                            window.location.reload();
                        }, 1000);
                    } else {
                        showNotification(data.message, 'error');
                    }
                })
                .catch(error => {
                    showNotification('Erro ao excluir: ' + error, 'error');
                })
                .finally(() => {
                    closeDeleteModal();
                });
            }
        }

        function refreshPage() {
            window.location.reload();
        }

        function updateFileName() {
            const fileInput = document.getElementById('fileInput');
            const fileName = document.getElementById('fileName');
            const uploadBtn = document.getElementById('uploadSubmitBtn');
            const fileDropText = document.getElementById('fileDropText');

            uploadBtn.addEventListener('click', function() {
                console.log('📤 Botão de upload clicado');
                console.log('📁 Arquivo selecionado:', fileInput.files[0]?.name);
                console.log('📍 Caminho de destino:', document.getElementById('uploadPath')?.value);
            });
            
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

        // ✅ NOVA FUNÇÃO: Tratamento de erro para download
        function handleDownloadError(filePath) {
            showNotification('Erro no download: ' + filePath, 'error');
            console.error('Falha no download:', filePath);
        }

        // ✅ Modificar a função de download para incluir tratamento de erro
        function downloadFileRobust(filePath) {
            if (!filePath) {
                showNotification('Caminho do arquivo inválido', 'error');
                return;
            }

            showNotification('Preparando download...', 'success');
            
            // ✅ Usar fetch para verificar se o download funciona
            fetch('/filesystem-download?path=' + encodeURIComponent(filePath))
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Erro HTTP: ' + response.status);
                    }
                    return response.blob();
                })
                .then(blob => {
                    // ✅ Criar link para download
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.style.display = 'none';
                    a.href = url;
                    a.download = filePath.split('/').pop();
                    
                    document.body.appendChild(a);
                    a.click();
                    
                    // ✅ Limpeza
                    window.URL.revokeObjectURL(url);
                    document.body.removeChild(a);
                    
                    showNotification('Download concluído: ' + a.download, 'success');
                })
                .catch(error => {
                    console.error('Erro no download:', error);
                    showNotification('Falha no download: ' + error.message, 'error');
                    
                    // ✅ Fallback: tentar método alternativo
                    setTimeout(() => {
                        downloadFileAlternative(filePath);
                    }, 1000);
                });
        }

        function formatFileSize(bytes) {
            if (bytes < 1024) return bytes + ' B';
            else if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
            else return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
        }

        // Upload form handler - VERSÃO CORRIGIDA

        document.getElementById('uploadForm').addEventListener('submit', function(e) {
            e.preventDefault();
            const fileInput = document.getElementById('fileInput');
            const uploadPath = document.getElementById('uploadPath').value.trim();
            
            if (fileInput.files.length === 0) {
                showNotification('Selecione um arquivo para upload', 'error');
                return;
            }

            if (!uploadPath || !uploadPath.startsWith('/')) {
                showNotification('Caminho inválido. Deve começar com /', 'error');
                return;
            }

            const formData = new FormData();
            formData.append('file', fileInput.files[0]);
            // ✅ REMOVER: formData.append('path', uploadPath); // Não funciona bem com multipart

            // ✅ CORREÇÃO: Usar parâmetro de query
            const url = '/filesystem-upload?path=' + encodeURIComponent(uploadPath);
            
            console.log('📤 Enviando upload para:', url);
            console.log('📁 Arquivo:', fileInput.files[0].name);

            fetch(url, {  // ✅ URL com parâmetro de query
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Erro HTTP: ' + response.status);
                }
                return response.json();
            })
            .then(data => {
                if (data.status === 'success') {
                    showNotification(data.message, 'success');
                    closeUploadModal();
                    setTimeout(() => {
                        window.location.reload();
                    }, 1000);
                } else {
                    showNotification(data.message, 'error');
                }
            })
            .catch(error => {
                console.error('❌ Erro no upload:', error);
                showNotification('Erro no upload: ' + error, 'error');
            });
        });

        // Create folder form handler
        document.getElementById('createFolderForm').addEventListener('submit', function(e) {
            e.preventDefault();
            const folderName = document.getElementById('folderName').value.trim();
            const folderPath = document.getElementById('folderPath').value.trim();
            
            if (!folderName) {
                showNotification('Digite um nome para a pasta', 'error');
                return;
            }

            if (!folderPath || !folderPath.startsWith('/')) {
                showNotification('Caminho inválido. Deve começar com /', 'error');
                return;
            }

            const basePath = folderPath.endsWith('/') ? folderPath : folderPath + '/';
            const fullPath = basePath + folderName;

            fetch('/filesystem-mkdir', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: `path=${encodeURIComponent(fullPath)}`
            })
            .then(response => response.json())
            .then(data => {
                if (data.status === 'success') {
                    showNotification(data.message, 'success');
                    closeCreateFolderModal();
                    setTimeout(() => {
                        window.location.reload();
                    }, 1000);
                } else {
                    showNotification(data.message, 'error');
                }
            })
            .catch(error => {
                showNotification('Erro ao criar pasta: ' + error, 'error');
            });
        });

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

        // Notification system
        function showNotification(message, type) {
            const existingNotifications = document.querySelectorAll('.notification');
            existingNotifications.forEach(notification => notification.remove());

            const notification = document.createElement('div');
            notification.className = `notification notification-${type}`;
            notification.innerHTML = `
                <div class="notification-content">
                    <span class="notification-icon">${type === 'success' ? '✅' : '❌'}</span>
                    <span class="notification-message">${message}</span>
                    <button class="notification-close" onclick="this.parentElement.parentElement.remove()">×</button>
                </div>
            `;

            if (!document.querySelector('#notification-styles')) {
                const styles = document.createElement('style');
                styles.id = 'notification-styles';
                styles.textContent = `
                    .notification {
                        position: fixed;
                        top: 20px;
                        right: 20px;
                        z-index: 10000;
                        background: var(--bg-card);
                        border: 1px solid var(--border-color);
                        border-radius: 0.5rem;
                        box-shadow: var(--shadow);
                        padding: 1rem;
                        min-width: 300px;
                        max-width: 500px;
                        animation: slideIn 0.3s ease;
                    }
                    .notification-success {
                        border-left: 4px solid var(--success);
                    }
                    .notification-error {
                        border-left: 4px solid var(--error);
                    }
                    .notification-content {
                        display: flex;
                        align-items: center;
                        gap: 0.5rem;
                    }
                    .notification-icon {
                        font-size: 1.2rem;
                    }
                    .notification-message {
                        flex: 1;
                        color: var(--text-primary);
                    }
                    .notification-close {
                        background: none;
                        border: none;
                        font-size: 1.5rem;
                        cursor: pointer;
                        color: var(--text-secondary);
                    }
                    @keyframes slideIn {
                        from { transform: translateX(100%); opacity: 0; }
                        to { transform: translateX(0); opacity: 1; }
                    }
                `;
                document.head.appendChild(styles);
            }

            document.body.appendChild(notification);

            setTimeout(() => {
                if (notification.parentElement) {
                    notification.remove();
                }
            }, 5000);
        }

        // Close modals when clicking outside
        window.onclick = function(event) {
            const modals = document.getElementsByClassName('modal');
            for (let modal of modals) {
                if (event.target === modal) {
                    modal.style.display = 'none';
                }
            }
        }

        // Initialize when page loads
        document.addEventListener('DOMContentLoaded', function() {
            initializeDragAndDrop();
            
            document.getElementById('uploadPath').value = currentDirectoryPath;
            document.getElementById('folderPath').value = currentDirectoryPath;

            // ✅ CORREÇÃO CRÍTICA: Prevenir que cliques em links abram o modal de delete
            document.addEventListener('click', function(e) {
                // Se o clique foi em um link de navegação (que contém href), não faz nada
                if (e.target.tagName === 'A' && e.target.getAttribute('href') && 
                    e.target.getAttribute('href').includes('/filesystem?path=')) {
                    // Permite a navegação normal - não interfere
                    return;
                }
                
                // Se o clique foi no botão de delete, mostra o modal
                if (e.target.classList.contains('btn-danger') || 
                    (e.target.parentElement && e.target.parentElement.classList.contains('btn-danger'))) {
                    // O evento de delete já é tratado pelo onclick do botão
                    return;
                }
            });
        });

        // ✅ NOVA FUNÇÃO: Navegar para uma pasta específica
        function navigateToFolder(path) {
            if (path && path.startsWith('/')) {
                window.location.href = '/filesystem?path=' + encodeURIComponent(path);
            }
        }
    </script>
</body>
</html>
)rawliteral";