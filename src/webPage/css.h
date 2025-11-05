#pragma once

const char *cssStyles = R"rawliteral(
:root {
    --bg-primary: #ffffff;
    --bg-secondary: #f8f9fa;
    --bg-card: #ffffff;
    --text-primary: #2d3748;
    --text-secondary: #4a5568;
    --accent-primary: #007cba;
    --accent-hover: #005a87;
    --border-color: #e2e8f0;
    --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
    --success: #28a745;
    --warning: #ffc107;
    --error: #dc3545;
}

body.dark {
    --bg-primary: #1a202c;
    --bg-secondary: #2d3748;
    --bg-card: #2d3748;
    --text-primary: #f7fafc;
    --text-secondary: #e2e8f0;
    --accent-primary: #63b3ed;
    --accent-hover: #4299e1;
    --border-color: #4a5568;
    --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3);
    --success: #68d391;
    --warning: #faf089;
    --error: #fc8181;
}

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: var(--bg-primary);
    color: var(--text-primary);
    line-height: 1.6;
    transition: all 0.3s ease;
}

.container {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
}

.navbar {
    background: var(--bg-card);
    padding: 1rem 2rem;
    box-shadow: var(--shadow);
    display: flex;
    justify-content: space-between;
    align-items: center;
}

.nav-brand {
    font-size: 1.5rem;
    font-weight: bold;
    color: var(--accent-primary);
    text-decoration: none;
}

.nav-links {
    display: flex;
    align-items: center;
    gap: 1rem;
}

.nav-links a {
    color: var(--text-primary);
    text-decoration: none;
    padding: 0.5rem 1rem;
    border-radius: 0.375rem;
    transition: background 0.2s;
}

.nav-links a:hover {
    background: var(--bg-secondary);
}

.theme-toggle {
    background: var(--bg-secondary);
    border: none;
    padding: 0.5rem;
    border-radius: 50%;
    cursor: pointer;
    font-size: 1.2rem;
    transition: transform 0.2s;
}

.theme-toggle:hover {
    transform: scale(1.1);
}

.main-content {
    flex: 1;
    padding: 2rem;
    max-width: 1200px;
    margin: 0 auto;
    width: 100%;
}

.header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    margin-bottom: 2rem;
    gap: 2rem;
}

.header-left {
    flex: 1;
}

.header h1 {
    margin: 0 0 0.5rem 0;
    color: var(--accent-primary);
}

.time-info {
    display: flex;
    flex-direction: column;
    gap: 0.25rem;
}

.system-time {
    font-size: 1rem;
    color: var(--text-primary);
    font-weight: 500;
}

.uptime {
    font-size: 0.9rem;
    color: var(--text-secondary);
}

.info-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 1.5rem;
    margin-bottom: 2rem;
}

.info-card {
    background: var(--bg-card);
    padding: 1.5rem;
    border-radius: 0.75rem;
    box-shadow: var(--shadow);
    border: 1px solid var(--border-color);
}

.info-card h3 {
    margin-bottom: 1rem;
    color: var(--accent-primary);
}

.action-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 1.5rem;
}

.action-card {
    background: var(--bg-card);
    padding: 2rem;
    border-radius: 0.75rem;
    box-shadow: var(--shadow);
    text-decoration: none;
    color: var(--text-primary);
    border: 2px solid transparent;
    transition: all 0.3s ease;
    text-align: center;
}

.action-card:hover {
    border-color: var(--accent-primary);
    transform: translateY(-2px);
    box-shadow: 0 8px 25px rgba(0, 0, 0, 0.15);
}

.action-icon {
    font-size: 3rem;
    margin-bottom: 1rem;
}

.footer {
    background: var(--bg-card);
    padding: 1rem 2rem;
    text-align: center;
    color: var(--text-secondary);
    border-top: 1px solid var(--border-color);
}

.upload-form {
    background: var(--bg-card);
    padding: 2rem;
    border-radius: 0.75rem;
    box-shadow: var(--shadow);
    max-width: 500px;
    margin: 0 auto;
}

.file-input {
    width: 100%;
    padding: 1rem;
    border: 2px dashed var(--border-color);
    border-radius: 0.5rem;
    margin: 1rem 0;
    text-align: center;
    cursor: pointer;
    transition: border-color 0.3s;
}

.file-input:hover {
    border-color: var(--accent-primary);
}

.btn {
    background: var(--accent-primary);
    color: white;
    padding: 0.75rem 1.5rem;
    border: none;
    border-radius: 0.5rem;
    cursor: pointer;
    font-size: 1rem;
    transition: background 0.2s;
    width: 100%;
}

.btn:hover {
    background: var(--accent-hover);
}

.btn:disabled {
    background: var(--text-secondary);
    cursor: not-allowed;
}

.progress {
    width: 100%;
    background: var(--bg-secondary);
    border-radius: 0.5rem;
    margin: 1rem 0;
    overflow: hidden;
}

.progress-bar {
    background: var(--accent-primary);
    height: 20px;
    border-radius: 0.5rem;
    width: 0%;
    transition: width 0.3s;
}

.success-message {
    text-align: center;
    padding: 3rem;
    background: var(--bg-card);
    border-radius: 0.75rem;
    box-shadow: var(--shadow);
}

@media (max-width: 768px) {
    .navbar {
        padding: 1rem;
        flex-direction: column;
        gap: 1rem;
    }
    
    .main-content {
        padding: 1rem;
    }
    
    .header {
        flex-direction: column;
        align-items: flex-start;
        gap: 1rem;
    }
    
    .info-grid {
        grid-template-columns: 1fr;
    }
}
)rawliteral";
