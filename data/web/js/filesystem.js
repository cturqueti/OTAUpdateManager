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
        .then(files => {
            displayRealFiles(files, path);
        })
        .catch(error => {
            console.error('Error loading files:', error);
            showStatus('Error loading files: ' + error.message, 'error');
            document.getElementById('fileList').innerHTML = '<div class="file-item error"><div class="file-name">Error loading files</div></div>';
        });
}

function displayRealFiles(files, currentPath) {
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
    
    // Display real files from LittleFS
    if (files.length === 0) {
        const emptyItem = document.createElement('div');
        emptyItem.className = 'file-item empty';
        emptyItem.innerHTML = `
            <div class="file-name">No files found</div>
            <div class="file-size">-</div>
            <div class="file-actions">-</div>
        `;
        fileList.appendChild(emptyItem);
        return;
    }
    
    files.forEach(item => {
        const fileItem = document.createElement('div');
        fileItem.className = `file-item ${item.isDirectory ? 'folder' : 'file'}`;
        
        const fullPath = currentPath + item.name + (item.isDirectory ? '/' : '');
        const displaySize = item.isDirectory ? '-' : formatFileSize(item.size);
        const icon = item.isDirectory ? '📁' : '📄';
        
        fileItem.innerHTML = `
            <div class="file-name" onclick="${item.isDirectory ? `loadRealFileList('${fullPath}')` : `downloadFile('${currentPath + item.name}')`}">
                ${icon} ${item.name}
            </div>
            <div class="file-size">${displaySize}</div>
            <div class="file-actions">
                ${item.isDirectory ? 
                    `<button class="btn-icon" onclick="loadRealFileList('${fullPath}')" title="Open">📂</button>` :
                    `<button class="btn-icon" onclick="downloadFile('${currentPath + item.name}')" title="Download">⬇️</button>`
                }
                <button class="btn-icon" onclick="showDeleteModal('${currentPath + item.name}')" title="Delete">🗑️</button>
            </div>
        `;
        fileList.appendChild(fileItem);
    });
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
        .then(response => response.text())
        .then(status => {
            console.log('File System Status:', status);
            showStatus('File System Status: ' + status, 'info');
        })
        .catch(error => {
            console.error('Error checking file system:', error);
            showStatus('Error checking file system: ' + error, 'error');
        });
}

// Chame esta função no carregamento da página para debug
document.addEventListener('DOMContentLoaded', function() {
    loadRealFileList('/');
    updateHostInfo();
    // testFileSystemPermissions(); 
});