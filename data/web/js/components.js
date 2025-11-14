// Função para carregar componentes
async function loadComponent(componentId, filePath) {
    try {
        const response = await fetch(filePath);
        if (!response.ok) throw new Error(`Erro ao carregar ${filePath}`);
        
        const html = await response.text();
        const element = document.getElementById(componentId);
        if (element) {
            element.innerHTML = html;
        }
    } catch (error) {
        console.error('Erro ao carregar componente:', error);
    }
}

// Função específica para navbar
// components.js - versão com debug
async function loadNavbar() {
    console.log('🚀 Iniciando carregamento da navbar...');
    
    try {
        const response = await fetch('/components/navbar.html');
        console.log('📡 Status da resposta:', response.status, response.ok);
        
        if (!response.ok) throw new Error('Erro ao carregar navbar');
        
        const navbarHtml = await response.text();
        console.log('📄 HTML carregado:', navbarHtml.substring(0, 100) + '...');
        
        const navbarElement = document.getElementById('navbar-container');
        if (navbarElement) {
            navbarElement.innerHTML = navbarHtml;
            console.log('✅ Navbar inserida no DOM com sucesso!');
            
            // Verificar se os elementos foram criados
            setTimeout(() => {
                const hamburger = document.querySelector('.hamburger-menu');
                const dropdown = document.getElementById('dropdownMenu');
                console.log('🍔 Hamburger encontrado:', !!hamburger);
                console.log('📋 Dropdown encontrado:', !!dropdown);
            }, 100);
            
        } else {
            console.error('❌ Elemento navbar-container não encontrado');
        }
    } catch (error) {
        console.error('❌ Erro ao carregar navbar:', error);
        // Fallback
        const navbarElement = document.getElementById('navbar-container');
        if (navbarElement) {
            navbarElement.innerHTML = `
                <nav class="navbar">
                    <a href="/" class="nav-brand">OTA Update</a>
                    <div class="nav-links">
                        <a href="/">Home</a>
                        <a href="/update.html">Upload</a>
                        <a href="/system.html">System</a>
                        <button class="theme-toggle nav-toggle" onclick="toggleTheme()">☀️</button>
                    </div>
                    <div class="hamburger-menu" onclick="toggleMenu()">
                        <div class="hamburger-line"></div>
                        <div class="hamburger-line"></div>
                        <div class="hamburger-line"></div>
                    </div>
                </nav>
            `;
            console.log('🔄 Usando navbar fallback');
        }
    }
}

function toggleMenu() {
    console.log('🍔 Hamburger clicado!');
    
    const dropdownMenu = document.getElementById('dropdownMenu');
    const hamburgerMenu = document.querySelector('.hamburger-menu');
    
    console.log('📋 Dropdown antes:', dropdownMenu.classList.contains('show'));
    console.log('🍔 Hamburger antes:', hamburgerMenu.classList.contains('active'));
    
    if (dropdownMenu.classList.contains('show')) {
        dropdownMenu.classList.remove('show');
        hamburgerMenu.classList.remove('active');
        console.log('❌ Fechando menu');
    } else {
        dropdownMenu.classList.add('show');
        hamburgerMenu.classList.add('active');
        console.log('✅ Abrindo menu');
        
        // Fechar menu ao clicar fora
        setTimeout(() => {
            document.addEventListener('click', closeMenuOnClickOutside);
        }, 10);
    }
}

// Restante do código permanece igual...
function toggleMenu() {
    const dropdownMenu = document.getElementById('dropdownMenu');
    const hamburgerMenu = document.querySelector('.hamburger-menu');
    
    if (dropdownMenu.classList.contains('show')) {
        dropdownMenu.classList.remove('show');
        hamburgerMenu.classList.remove('active');
    } else {
        dropdownMenu.classList.add('show');
        hamburgerMenu.classList.add('active');
        
        // Fechar menu ao clicar fora
        setTimeout(() => {
            document.addEventListener('click', closeMenuOnClickOutside);
        }, 10);
    }
}

function closeMenuOnClickOutside(event) {
    const dropdownMenu = document.getElementById('dropdownMenu');
    const hamburgerMenu = document.querySelector('.hamburger-menu');
    
    if (!dropdownMenu.contains(event.target) && !hamburgerMenu.contains(event.target)) {
        dropdownMenu.classList.remove('show');
        hamburgerMenu.classList.remove('active');
        document.removeEventListener('click', closeMenuOnClickOutside);
    }
}

document.addEventListener('keydown', function(event) {
    if (event.key === 'Escape') {
        const dropdownMenu = document.getElementById('dropdownMenu');
        const hamburgerMenu = document.querySelector('.hamburger-menu');
        
        dropdownMenu.classList.remove('show');
        hamburgerMenu.classList.remove('active');
        document.removeEventListener('click', closeMenuOnClickOutside);
    }
});

// Carregar navbar quando o DOM estiver pronto
document.addEventListener('DOMContentLoaded', function() {
    loadNavbar();
});