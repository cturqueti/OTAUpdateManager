
// Update page specific JavaScript
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
