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
)rawliteral";