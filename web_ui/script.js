document.addEventListener('DOMContentLoaded', () => {
    const terminalBody = document.getElementById('terminal-body');
    const terminalInput = document.getElementById('terminal-input');
    const terminalContainer = document.getElementById('terminal-container');
    const themeToggle = document.getElementById('theme-toggle');
    const body = document.body;

    let currentPromptLine = null;
    let commandHistory = [];
    let historyIndex = -1;

    // --- Virtual File System ---
    let fs = {
        name: '/',
        type: 'dir',
        children: [
            { name: 'bin', type: 'dir', children: [] },
            { name: 'home', type: 'dir', children: [
                { name: 'guest', type: 'dir', children: [
                    { name: 'projects', type: 'dir', children: [] },
                    { name: 'readme.txt', type: 'file', content: 'Welcome to the Mini Linux Shell Web UI!\nThis is a simulation of the C-based project.' },
                    { name: 'todo.md', type: 'file', content: '- Implement pipes\n- Finish UI styling\n- Submit project' }
                ]}
            ]},
            { name: 'etc', type: 'dir', children: [] },
            { name: 'var', type: 'dir', children: [] }
        ]
    };

    let currentPath = ['home', 'guest'];
    let isEditorMode = false;
    let currentEditingFile = null;

    const nanoEditor = document.getElementById('nano-editor');
    const nanoTextarea = document.getElementById('nano-textarea');
    const nanoFilename = document.getElementById('nano-filename');

    function getDir(path) {
        let curr = fs;
        for (const p of path) {
            const found = curr.children.find(c => c.name === p && c.type === 'dir');
            if (!found) return null;
            curr = found;
        }
        return curr;
    }

    function resolvePath(pathStr) {
        if (!pathStr || pathStr === '~') return ['home', 'guest'];
        if (pathStr === '/') return [];
        
        let parts = pathStr.split('/').filter(p => p !== '' && p !== '.');
        let newPath = pathStr.startsWith('/') ? [] : [...currentPath];

        for (const p of parts) {
            if (p === '..') {
                if (newPath.length > 0) newPath.pop();
            } else {
                newPath.push(p);
            }
        }
        return newPath;
    }

    // --- Editor Logic ---
    function openNano(filename) {
        const dir = getDir(currentPath);
        const file = dir.children.find(c => c.name === filename && c.type === 'file');
        
        isEditorMode = true;
        currentEditingFile = filename;
        nanoFilename.textContent = filename;
        nanoTextarea.value = file ? file.content : '';
        
        nanoEditor.style.display = 'flex';
        terminalBody.style.display = 'none';
        nanoTextarea.focus();
    }

    function closeNano() {
        isEditorMode = false;
        currentEditingFile = null;
        nanoEditor.style.display = 'none';
        terminalBody.style.display = 'block';
        terminalInput.focus();
        showPrompt();
    }

    function saveNano() {
        const dir = getDir(currentPath);
        let file = dir.children.find(c => c.name === currentEditingFile && c.type === 'file');
        if (!file) {
            file = { name: currentEditingFile, type: 'file', content: '' };
            dir.children.push(file);
        }
        file.content = nanoTextarea.value;
    }

    // --- Theme Management ---
    const savedTheme = localStorage.getItem('theme') || 'dark';
    body.setAttribute('data-theme', savedTheme);
    updateThemeIcon(savedTheme);

    themeToggle.addEventListener('click', () => {
        const currentTheme = body.getAttribute('data-theme');
        const newTheme = currentTheme === 'dark' ? 'light' : 'dark';
        body.setAttribute('data-theme', newTheme);
        localStorage.setItem('theme', newTheme);
        updateThemeIcon(newTheme);
    });

    function updateThemeIcon(theme) {
        themeToggle.textContent = theme === 'dark' ? '🌙' : '☀️';
    }

    // --- Terminal Interaction ---
    window.addEventListener('keydown', (e) => {
        if (isEditorMode) {
            if (e.ctrlKey && e.key === 'x') {
                e.preventDefault();
                closeNano();
            } else if (e.ctrlKey && e.key === 'o') {
                e.preventDefault();
                saveNano();
            }
        }
    });

    terminalContainer.addEventListener('click', () => {
        if (isEditorMode) nanoTextarea.focus();
        else terminalInput.focus();
    });

    terminalInput.addEventListener('input', () => {
        if (isEditorMode) return;
        if (currentPromptLine) {
            const commandText = currentPromptLine.querySelector('.command-text');
            commandText.textContent = terminalInput.value;
        }
    });

    terminalInput.addEventListener('keydown', (e) => {
        if (isEditorMode) return;
        if (e.key === 'Enter') {
            const command = terminalInput.value.trim();
            executeCommand(command);
            terminalInput.value = '';
        } else if (e.key === 'ArrowUp') {
            if (commandHistory.length > 0) {
                historyIndex = Math.min(historyIndex + 1, commandHistory.length - 1);
                terminalInput.value = commandHistory[commandHistory.length - 1 - historyIndex];
                updateCommandDisplay();
            }
        } else if (e.key === 'ArrowDown') {
            if (historyIndex > 0) {
                historyIndex--;
                terminalInput.value = commandHistory[commandHistory.length - 1 - historyIndex];
            } else {
                historyIndex = -1;
                terminalInput.value = '';
            }
            updateCommandDisplay();
        }
    });

    function updateCommandDisplay() {
        if (currentPromptLine) {
            const commandText = currentPromptLine.querySelector('.command-text');
            commandText.textContent = terminalInput.value;
        }
    }

    // --- Command Execution Logic ---
    async function executeCommand(input) {
        const cursor = currentPromptLine.querySelector('.cursor');
        if (cursor) cursor.remove();

        if (input !== '') {
            commandHistory.push(input);
            historyIndex = -1;

            const parts = input.split(' ');
            const cmd = parts[0].toLowerCase();
            const args = parts.slice(1);

            switch (cmd) {
                case 'help':
                    appendStaticLine('Available Commands:');
                    appendStaticLine('  Navigation: cd, pwd, ls');
                    appendStaticLine('  File Ops:   mkdir, touch, rm, cat, echo');
                    appendStaticLine('  System:     date, whoami, uname, hostname, clear, history');
                    appendStaticLine('  Misc:       about, exit');
                    break;
                case 'about':
                    appendStaticLine('Mini Linux Shell v2.0.0');
                    appendStaticLine('Developed by Kushagra. A full-featured terminal simulation.');
                    break;
                case 'clear':
                    terminalBody.innerHTML = '';
                    break;
                case 'pwd':
                    appendStaticLine('/' + currentPath.join('/'));
                    break;
                case 'ls':
                    const dir = getDir(currentPath);
                    if (dir) {
                        const names = dir.children.map(c => c.type === 'dir' ? c.name + '/' : c.name);
                        appendStaticLine(names.join('  '));
                    }
                    break;
                case 'cd':
                    const newPath = resolvePath(args[0]);
                    if (getDir(newPath)) {
                        currentPath = newPath;
                    } else {
                        appendStaticLine(`cd: no such file or directory: ${args[0]}`);
                    }
                    break;
                case 'mkdir':
                    if (!args[0]) { appendStaticLine('usage: mkdir <directory>'); break; }
                    const parent = getDir(currentPath);
                    if (parent.children.find(c => c.name === args[0])) {
                        appendStaticLine(`mkdir: ${args[0]}: already exists`);
                    } else {
                        parent.children.push({ name: args[0], type: 'dir', children: [] });
                    }
                    break;
                case 'touch':
                    if (!args[0]) { appendStaticLine('usage: touch <file>'); break; }
                    const pDir = getDir(currentPath);
                    if (!pDir.children.find(c => c.name === args[0])) {
                        pDir.children.push({ name: args[0], type: 'file', content: '' });
                    }
                    break;
                case 'cat':
                    if (!args[0]) { appendStaticLine('usage: cat <file>'); break; }
                    const d = getDir(currentPath);
                    const f = d.children.find(c => c.name === args[0] && c.type === 'file');
                    if (f) appendStaticLine(f.content);
                    else appendStaticLine(`cat: ${args[0]}: no such file`);
                    break;
                case 'rm':
                    if (!args[0]) { appendStaticLine('usage: rm <file/dir>'); break; }
                    const targetDir = getDir(currentPath);
                    const idx = targetDir.children.findIndex(c => c.name === args[0]);
                    if (idx !== -1) targetDir.children.splice(idx, 1);
                    else appendStaticLine(`rm: ${args[0]}: no such file or directory`);
                    break;
                case 'date':
                    appendStaticLine(new Date().toString());
                    break;
                case 'whoami':
                    appendStaticLine('guest');
                    break;
                case 'uname':
                    appendStaticLine('Linux mls-web-sim 5.15.0-generic #54-Ubuntu SMP');
                    break;
                case 'hostname':
                    appendStaticLine('mls-web-terminal');
                    break;
                case 'echo':
                    appendStaticLine(args.join(' '));
                    break;
                case 'history':
                    commandHistory.forEach((h, i) => appendStaticLine(`${i + 1}  ${h}`));
                    break;
                case 'exit':
                    appendStaticLine('Session ended. Reloading...');
                    setTimeout(() => location.reload(), 1500);
                    break;
                case 'nano':
                    if (!args[0]) { appendStaticLine('usage: nano <file>'); break; }
                    openNano(args[0]);
                    return; // Don't show prompt immediately
                default:
                    appendStaticLine(`Error: command not found: ${cmd}`);
            }
        }

        showPrompt();
    }

    // --- Output Utilities ---
    const bootSequence = [
        { text: 'Initializing Mini Linux Shell v2.0.0...', delay: 300 },
        { text: 'Checking system integrity... [OK]', delay: 500 },
        { text: 'Loading virtual filesystem... [OK]', delay: 700 },
        { text: 'System ready.', delay: 400 },
        { text: '', delay: 200 }
    ];

    async function runBootSequence() {
        for (const line of bootSequence) {
            await appendTerminalLine(line.text, line.delay);
        }
        showPrompt();
    }

    function appendTerminalLine(text, delay = 0) {
        return new Promise((resolve) => {
            setTimeout(() => {
                const lineDiv = document.createElement('div');
                lineDiv.className = 'terminal-line';
                terminalBody.appendChild(lineDiv);
                if (text === '') { resolve(); return; }

                let i = 0;
                const interval = setInterval(() => {
                    lineDiv.textContent += text[i];
                    i++;
                    if (i === text.length) {
                        clearInterval(interval);
                        terminalBody.scrollTop = terminalBody.scrollHeight;
                        resolve();
                    }
                }, 10);
            }, delay);
        });
    }

    function appendStaticLine(text) {
        const lineDiv = document.createElement('div');
        lineDiv.className = 'terminal-line';
        lineDiv.textContent = text;
        terminalBody.appendChild(lineDiv);
        terminalBody.scrollTop = terminalBody.scrollHeight;
    }

    function showPrompt() {
        const promptDiv = document.createElement('div');
        promptDiv.className = 'terminal-line';
        const displayPath = currentPath.length === 0 ? '/' : (currentPath[0] === 'home' && currentPath[1] === 'guest' ? '~' + (currentPath.length > 2 ? '/' + currentPath.slice(2).join('/') : '') : '/' + currentPath.join('/'));
        promptDiv.innerHTML = `<span class="prompt">guest@mls:${displayPath}$</span><span class="command-text"></span><span class="cursor"></span>`;
        terminalBody.appendChild(promptDiv);
        currentPromptLine = promptDiv;
        terminalBody.scrollTop = terminalBody.scrollHeight;
        terminalInput.focus();
    }

    runBootSequence();
});
