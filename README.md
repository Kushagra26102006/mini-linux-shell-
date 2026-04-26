# 🐧 Mini Linux Shell v2.1 (Expert Edition)

A high-fidelity Linux shell implementation in C, designed for power users and academic demonstrations. This version introduces advanced multi-stage piping, quote-aware parsing, and native TAB completion without external library dependencies.

## 🚀 Key Improvements

- **Multi-Pipe Engine**: Chain any number of commands together (e.g., `ls | grep .c | wc -l`).
- **Quote-Aware Parser**: Handles spaces inside single and double quotes correctly (e.g., `echo "hello world"`).
- **Tab Autocomplete**: Native implementation for command and filename completion using raw terminal mode.
- **Enhanced 'ls'**: Full support for `-l` (long listing) and `-a` (all files) with permissions and owner info.
- **Expert Codebase**: Modular C11 code with 30+ internal commands and professional UI polish.

## 🛠️ Usage

### Build
```bash
make
```

### Commands
- **Navigation**: `cd`, `pwd`, `ls -la`
- **File Management**: `mkdir`, `rm`, `cp`, `mv`, `touch`, `stat`
- **Processing**: `head`, `tail`, `wc`, `cat`
- **System**: `date`, `whoami`, `uname`, `env`, `export`, `kill`

## 🖥️ Web UI
An interactive terminal simulation is available in the `web_ui/` directory for browser-based demonstrations.

---
*Built with C11, System Calls, and ANSI Power.*
