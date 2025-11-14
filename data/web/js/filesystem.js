// File system specific JavaScript
let currentPath = '/';
let itemToDelete = '';

// Load files when page loads
document.addEventListener('DOMContentLoaded', function() {
    loadRealFileList('/');
    updateHostInfo();
});

function loadRealFileList(path) {
    currentPath = path;
    document.getElementById('currentPath').textContent = path;
    document.getElementById('fileList').innerHTML = '<div class="file-item loading"><div class="file-name">Loading files...</div></div>';
    
    fetch('/filesystem-list?path=' + encodeURIComponent(path))
        .then(response => {
            if (!response.ok) {
                throw new Error('Directory not found');
            }
            return response.json();
        })
        .then(data => {
            if (data.success) {
                displayRealFiles(data, path);
            } else {
                throw new Error(data.error || 'Unknown error');
            }
        })
        .catch(error => {
            console.error('Error loading files:', error);
            showStatus('Error loading files: ' + error.message, 'error');
            document.getElementById('fileList').innerHTML = '<div class="file-item error"><div class="file-name">Error loading files</div></div>';
        });
}

function displayRealFiles(data, currentPath) {
    const fileList = document.getElementById('fileList');
    fileList.innerHTML = '';
    
    // Add parent directory link (except for root)
    if (currentPath !== '/') {
        const pathParts = currentPath.split('/').filter(part => part !== '');
        pathParts.pop(); // Remove last part
        const parentPath = pathParts.length > 0 ? '/' + pathParts.join('/') + '/' : '/';
        
        const parentItem = document.createElement('div');
        parentItem.className = 'file-item folder';
        parentItem.innerHTML = `
            <div class="file-name" onclick="loadRealFileList('${parentPath}')">📁 ..</div>
            <div class="file-size">-</div>
            <div class="file-actions">-</div>
        `;
        fileList.appendChild(parentItem);
    }
    
    // Display files from JSON response
    const files = data.files || [];
    
    if (files.length === 0) {
        const emptyItem = document.createElement('div');
        emptyItem.className = 'file-item empty';
        emptyItem.innerHTML = `
            <div class="file-name">No files found</div>
            <div class="file-size">-</div>
            <div class="file-actions">-</div>
        `;
        fileList.appendChild(emptyItem);
        
        // Show directory info
        showStatus(`Directory: ${currentPath} | Files: 0 | Total Size: ${formatBytes(data.totalSize || 0)}`, 'info');
        return;
    }
    
    files.forEach(item => {
        const fileItem = document.createElement('div');
        fileItem.className = `file-item ${item.isDirectory ? 'folder' : 'file'}`;
        
        // Use the path from JSON or construct it
        const fullPath = item.path || (currentPath + item.name + (item.isDirectory ? '/' : ''));
        const displaySize = item.isDirectory ? '-' : formatBytes(item.size);
        const icon = item.isDirectory ? '📁' : '📄';
        
        fileItem.innerHTML = `
            <div class="file-name" onclick="${item.isDirectory ? `loadRealFileList('${fullPath}')` : `downloadFile('${fullPath}')`}">
                ${icon} ${item.name}
                ${item.extension ? `<span class="file-extension">.${item.extension}</span>` : ''}
            </div>
            <div class="file-size">${displaySize}</div>
            <div class="file-actions">
                ${item.isDirectory ? 
                    `<button class="btn-icon" onclick="loadRealFileList('${fullPath}')" title="Open">📂</button>` :
                    `<button class="btn-icon" onclick="downloadFile('${fullPath}')" title="Download">⬇️</button>`
                }
                <button class="btn-icon" onclick="showDeleteModal('${fullPath}')" title="Delete">🗑️</button>
                ${!item.isDirectory ? `<button class="btn-icon" onclick="showFileInfo('${fullPath}')" title="Info">ℹ️</button>` : ''}
            </div>
        `;
        fileList.appendChild(fileItem);
    });
    
    // Show directory info
    showStatus(`Directory: ${currentPath} | Files: ${data.count || files.length} | Total Size: ${formatBytes(data.totalSize || 0)}`, 'info');
}

function showCreateFolder() {
    document.getElementById('folderNameInput').value = '';
    showModal('createFolderModal');
}

function createFolder() {
    const folderName = document.getElementById('folderNameInput').value.trim();
    if (!folderName) {
        showStatus('Please enter a folder name', 'error');
        return;
    }
    
    // Construir path corretamente
    let fullPath = currentPath + folderName;
    fullPath = fullPath.replace(/\/+/g, '/'); // Remove barras duplicadas
    
    console.log('Creating folder:', fullPath);
    
    fetch('/filesystem-mkdir', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'path=' + encodeURIComponent(fullPath)
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('Server error: ' + response.status);
        }
        return response.text();
    })
    .then(result => {
        showStatus('Folder created: ' + result, 'success');
        closeModal('createFolderModal');
        setTimeout(() => loadRealFileList(currentPath), 1000);
    })
    .catch(error => {
        showStatus('Error creating folder: ' + error.message, 'error');
    });
}

function uploadFiles() {
    const files = document.getElementById('fileInput').files;
    if (files.length === 0) {
        showStatus('Please select files to upload', 'error');
        return;
    }

    showModal('uploadModal');
    const formData = new FormData();
    
    for (let file of files) {
        formData.append('file', file);
    }

    fetch('/filesystem-upload?path=' + encodeURIComponent(currentPath), {
        method: 'POST',
        body: formData
    })
    .then(response => response.text())
    .then(result => {
        closeModal('uploadModal');
        showStatus('Upload successful: ' + result, 'success');
        document.getElementById('fileInput').value = '';
        setTimeout(() => loadRealFileList(currentPath), 1000);
    })
    .catch(error => {
        closeModal('uploadModal');
        showStatus('Upload failed: ' + error, 'error');
    });
}

function showDeleteModal(itemPath) {
    itemToDelete = itemPath;
    document.getElementById('deleteItemName').textContent = itemPath;
    console.log('Delete item path:', itemPath);
    showModal('deleteModal');
}

function confirmDelete() {
    fetch('/filesystem-delete', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'path=' + encodeURIComponent(itemToDelete)
    })
    .then(response => response.text())
    .then(result => {
        showStatus(result, 'success');
        closeModal('deleteModal');
        setTimeout(() => loadRealFileList(currentPath), 500);
        itemToDelete = '';
    })
    .catch(error => {
        showStatus('Delete failed: ' + error, 'error');
        closeModal('deleteModal');
    });
}

function downloadFile(filePath) {
    showStatus('Downloading: ' + filePath, 'info');
    
    // Create temporary download link
    const downloadLink = document.createElement('a');
    downloadLink.href = '/filesystem-download?path=' + encodeURIComponent(filePath);
    downloadLink.download = filePath.split('/').pop();
    downloadLink.style.display = 'none';
    
    document.body.appendChild(downloadLink);
    downloadLink.click();
    document.body.removeChild(downloadLink);
    
    setTimeout(() => {
        showStatus('Download completed: ' + filePath, 'success');
    }, 1000);
}

// Nova função para mostrar informações do arquivo
function showFileInfo(filePath) {
    fetch('/api/filesystem-info?path=' + encodeURIComponent(filePath))
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                let infoHTML = `
                    <div style="text-align: left; line-height: 1.6;">
                        <h4 style="margin: 0 0 10px 0; color: var(--accent-primary);">📄 File Information</h4>
                        <div style="background: var(--bg-secondary); padding: 10px; border-radius: 4px; margin-bottom: 10px;">
                            <strong>${data.name}</strong>
                            ${data.extension ? `<span style="color: var(--text-tertiary);">.${data.extension}</span>` : ''}
                        </div>
                        <table style="width: 100%; font-size: 0.9em;">
                            <tr><td><strong>Path:</strong></td><td>${data.path}</td></tr>
                            <tr><td><strong>Type:</strong></td><td>${data.isDirectory ? '📁 Directory' : '📄 File'}</td></tr>
                            <tr><td><strong>Size:</strong></td><td>${formatBytes(data.size)}</td></tr>
                `;
                
                if (data.modified) {
                    infoHTML += `<tr><td><strong>Modified:</strong></td><td>${data.modified}</td></tr>`;
                }
                
                if (data.mimeType) {
                    infoHTML += `<tr><td><strong>MIME Type:</strong></td><td>${data.mimeType}</td></tr>`;
                }
                
                if (data.isDirectory) {
                    infoHTML += `
                        <tr><td><strong>Files:</strong></td><td>${data.fileCount || 0}</td></tr>
                        <tr><td><strong>Subdirectories:</strong></td><td>${data.dirCount || 0}</td></tr>
                        <tr><td><strong>Total Items:</strong></td><td>${data.totalItems || 0}</td></tr>
                        <tr><td><strong>Total Size:</strong></td><td>${formatBytes(data.totalSize || 0)}</td></tr>
                    `;
                }
                
                infoHTML += `
                        </table>
                        <div style="margin-top: 10px; padding: 8px; background: var(--bg-secondary); border-radius: 4px; font-size: 0.8em;">
                            <strong>File System:</strong> ${formatBytes(data.fsUsedBytes || 0)} / ${formatBytes(data.fsTotalBytes || 0)} used (${data.fsPercentUsed || 0}%)
                        </div>
                    </div>
                `;
                
                // Usar um modal ou alerta personalizado
                showCustomModal('File Information', infoHTML);
            } else {
                throw new Error(data.error || 'Failed to get file info');
            }
        })
        .catch(error => {
            showStatus('Error getting file info: ' + error.message, 'error');
        });
}

// Função auxiliar para mostrar modal personalizado
function showCustomModal(title, content) {
    // Você pode implementar um modal personalizado ou usar o alert
    const modal = document.createElement('div');
    modal.style.cssText = `
        position: fixed;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        background: var(--card-bg);
        border: 1px solid var(--border-color);
        border-radius: 8px;
        padding: 20px;
        z-index: 1000;
        max-width: 500px;
        width: 90%;
        box-shadow: 0 4px 20px rgba(0,0,0,0.3);
    `;
    
    modal.innerHTML = `
        <div style="display: flex; justify-content: between; align-items: center; margin-bottom: 15px;">
            <h3 style="margin: 0; color: var(--text-primary);">${title}</h3>
            <button onclick="this.parentElement.parentElement.remove()" style="background: none; border: none; font-size: 1.2rem; cursor: pointer; color: var(--text-secondary);">&times;</button>
        </div>
        <div>${content}</div>
        <div style="margin-top: 20px; text-align: right;">
            <button onclick="this.parentElement.parentElement.parentElement.remove()" style="padding: 8px 16px; background: var(--accent-primary); color: white; border: none; border-radius: 4px; cursor: pointer;">Close</button>
        </div>
    `;
    
    document.body.appendChild(modal);
    
    // Adicionar overlay
    const overlay = document.createElement('div');
    overlay.style.cssText = `
        position: fixed;
        top: 0;
        left: 0;
        width: 100%;
        height: 100%;
        background: rgba(0,0,0,0.5);
        z-index: 999;
    `;
    overlay.onclick = () => {
        modal.remove();
        overlay.remove();
    };
    document.body.appendChild(overlay);
}

function refreshFileList() {
    loadRealFileList(currentPath);
    showStatus('File list refreshed', 'info');
}

function updateHostInfo() {
    // Get host info from main script
    if (typeof getSystemInfo === 'function') {
        getSystemInfo();
    }
}

// Debug function
function debugCurrentState() {
    console.log('=== DEBUG FILE MANAGER ===');
    console.log('Current Path:', currentPath);
    const folderName = document.getElementById('folderNameInput').value;
    console.log('Folder Name Input:', folderName);
    console.log('Full Path to Create:', currentPath + folderName);
    console.log('==========================');
}
function testFileSystemPermissions() {
    showStatus('Testing file system permissions...', 'info');
    
    fetch('/filesystem-status')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                console.log('File System Status:', data);
                showStatus('File System Status: ' + (data.message || 'OK'), 'info');
            } else {
                throw new Error(data.error || 'Status check failed');
            }
        })
        .catch(error => {
            console.error('Error checking file system:', error);
            showStatus('Error checking file system: ' + error.message, 'error');
        });
}



// Chame esta função no carregamento da página para debug
document.addEventListener('DOMContentLoaded', function() {
    const style = document.createElement('style');
    style.textContent = additionalCSS;
    document.head.appendChild(style);
    
    loadRealFileList('/');
    updateHostInfo();
});

