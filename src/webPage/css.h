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

/* File Manager Styles */
.breadcrumb {
    background: var(--bg-card);
    padding: 0.75rem 1rem;
    border-radius: 0.375rem;
    margin-bottom: 1rem;
    font-size: 0.9rem;
}

.breadcrumb a {
    color: var(--accent-primary);
    text-decoration: none;
    margin-right: 0.5rem;
}

.breadcrumb a:hover {
    text-decoration: underline;
}

.file-actions {
    display: flex;
    gap: 0.5rem;
    margin-bottom: 1rem;
    flex-wrap: wrap;
}

.file-list-container {
    background: var(--bg-card); 
    border-radius: 0.375rem;
    padding: 1rem;
    margin-bottom: 1rem;
}

.file-list {
    width: 100%;
    border-collapse: collapse;
}

.file-list th,
.file-list td {
    padding: 0.75rem;
    text-align: left;
    border-bottom: 1px solid var(--border-color);
}

.file-list th {
    background: var(--bg-secondary);
    font-weight: 600;
}

.file-item:hover {
    background: var(--bg-secondary);
}

.file-icon {
    margin-right: 0.5rem;
    width: 20px;
    text-align: center;
}

.file-actions-cell {
    display: flex;
    gap: 0.25rem;
}

.btn-sm {
    padding: 0.25rem 0.5rem;
    font-size: 0.8rem;
}

.btn-danger {
    background: var(--error);
    color: white;
}

.btn-danger:hover {
    background: #dc2626;
}

.empty-state {
    text-align: center;
    padding: 2rem;
    color: var(--text-secondary);
}


.empty-state .icon {
    font-size: 3rem;
    margin-bottom: 1rem;
    opacity: 0.5;
}

/* Modal Styles */
.modal {
    display: none;
    position: fixed;
    z-index: 1000;
    left: 0;
    top: 0;
    width: 100%;
    height: 100%;
    background-color: rgba(0, 0, 0, 0.5);
}

.modal-content {
    background-color: var(--card-bg);
    margin: 5% auto;
    padding: 0;
    border-radius: 0.5rem;
    width: 90%;
    max-width: 500px;
    box-shadow: 0 10px 25px rgba(0, 0, 0, 0.2);
}

.modal-header {
    padding: 1rem 1.5rem;
    border-bottom: 1px solid var(--border-color);
    display: flex;
    justify-content: between;
    align-items: center;
}

.modal-header h3 {
    margin: 0;
    flex: 1;
}

.close {
    color: var(--text-secondary);
    font-size: 1.5rem;
    font-weight: bold;
    cursor: pointer;
}

.close:hover {
    color: var(--text-primary);
}

.modal-body {
    padding: 1.5rem;
}

.form-group {
    margin-bottom: 1rem;
}

.form-group label {
    display: block;
    margin-bottom: 0.5rem;
    font-weight: 500;
}

.form-group input {
    width: 100%;
    padding: 0.5rem;
    border: 1px solid var(--border-color);
    border-radius: 0.375rem;
    background: var(--bg-primary);
    color: var(--text-primary);
}

.form-actions {
    display: flex;
    gap: 0.5rem;
    justify-content: flex-end;
    margin-top: 1.5rem;
}

/* File Manager - Input Styles */
.form-group input, 
.form-group select, 
.form-group textarea {
    width: 100%;
    padding: 0.75rem;
    border: 1px solid var(--border-color);
    border-radius: 0.375rem;
    background: var(--bg-primary);
    color: var(--text-primary);
    font-size: 1rem;
    transition: border-color 0.3s, box-shadow 0.3s;
}

.form-group input:focus, 
.form-group select:focus, 
.form-group textarea:focus {
    outline: none;
    border-color: var(--accent-primary);
    box-shadow: 0 0 0 3px rgba(99, 179, 237, 0.1);
}

/* Estilo específico para inputs de arquivo */
input[type="file"] {
    padding: 1rem;
    border: 2px dashed var(--border-color);
    background: var(--bg-secondary);
    cursor: pointer;
    transition: all 0.3s ease;
}

input[type="file"]:hover {
    border-color: var(--accent-primary);
    background: var(--bg-primary);
}

input[type="file"]::file-selector-button {
    background: var(--accent-primary);
    color: white;
    padding: 0.5rem 1rem;
    border: none;
    border-radius: 0.25rem;
    cursor: pointer;
    margin-right: 1rem;
    transition: background 0.2s;
}

input[type="file"]::file-selector-button:hover {
    background: var(--accent-hover);
}

/* Placeholder styling */
.form-group input::placeholder {
    color: var(--text-secondary);
    opacity: 0.7;
}

/* Select dropdown styling */
.form-group select {
    appearance: none;
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='16' height='16' viewBox='0 0 24 24' fill='none' stroke='%234a5568' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3E%3Cpath d='M6 9l6 6 6-6'/%3E%3C/svg%3E");
    background-repeat: no-repeat;
    background-position: right 0.75rem center;
    background-size: 16px;
    padding-right: 2.5rem;
}

body.dark .form-group select {
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='16' height='16' viewBox='0 0 24 24' fill='none' stroke='%23e2e8f0' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3E%3Cpath d='M6 9l6 6 6-6'/%3E%3C/svg%3E");
}

/* Modal input focus states */
.modal-body .form-group input:focus,
.modal-body .form-group select:focus {
    border-color: var(--accent-primary);
    box-shadow: 0 0 0 3px rgba(99, 179, 237, 0.2);
}

/* Readonly inputs styling */
.form-group input[readonly] {
    background: var(--bg-secondary);
    color: var(--text-secondary);
    cursor: not-allowed;
}

/* File input drag & drop area */
.file-drop-area {
    border: 2px dashed var(--border-color);
    border-radius: 0.5rem;
    padding: 2rem;
    text-align: center;
    background: var(--bg-secondary);
    transition: all 0.3s ease;
    cursor: pointer;
}

.file-drop-area:hover,
.file-drop-area.dragover {
    border-color: var(--accent-primary);
    background: var(--bg-primary);
}

.file-drop-area p {
    margin: 0;
    color: var(--text-secondary);
}

.file-drop-area .icon {
    font-size: 2rem;
    margin-bottom: 0.5rem;
    opacity: 0.7;
}

/* Button states */
.btn:disabled {
    background: var(--text-secondary);
    cursor: not-allowed;
    opacity: 0.6;
}

.btn:disabled:hover {
    background: var(--text-secondary);
    transform: none;
}

/* Primary button */
.btn-primary {
    background: var(--accent-primary);
    color: white;
}

.btn-primary:hover:not(:disabled) {
    background: var(--accent-hover);
    transform: translateY(-1px);
}

.file-actions-cell .btn-sm {
    padding: 0.25rem 0.5rem;
    font-size: 0.8rem;
    margin: 0.1rem;
}

/* Secondary button */
.btn-secondary {
    background: var(--bg-secondary);
    color: var(--text-primary);
    border: 1px solid var(--border-color);
}

.btn-secondary:hover:not(:disabled) {
    background: var(--border-color);
}

/* Danger button */
.btn-danger {
    background: var(--error);
    color: white;
}

.btn-danger:hover:not(:disabled) {
    background: #dc2626;
    transform: translateY(-1px);
}


@media (max-width: 768px) {
    .file-actions {
        flex-direction: column;
    }
    
    .file-actions .btn {
        width: 100%;
    }
    
    .file-list {
        font-size: 0.8rem;
    }
    
    .modal-content {
        width: 95%;
        margin: 10% auto;
    }
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
