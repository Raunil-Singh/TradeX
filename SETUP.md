# Development Environment Setup

This guide will help you set up a consistent development environment across Windows, macOS, and Linux for completing the multi-threading tasks.

## Prerequisites

All mentees should work in a Unix-like environment to ensure consistency. This means:
- **Linux users**: Use your native environment
- **macOS users**: Use your native terminal (macOS is Unix-based)
- **Windows users**: Set up WSL2 (Windows Subsystem for Linux)

---

## Table of Contents

1. [Git Repository Setup](#git-repository-setup)
2. [Windows Setup (WSL2)](#windows-setup-wsl2)
3. [macOS Setup](#macos-setup)
4. [Linux Setup](#linux-setup)
5. [Common Tools Installation](#common-tools-installation)
6. [Verify Your Setup](#verify-your-setup)
7. [Workflow Guidelines](#workflow-guidelines)

---

## Git Repository Setup

### 1. Clone the Repository

```bash
git clone https://github.com/Raunil-Singh/TradeX.git
cd TradeX
```

### 2. Switch to the Learning Phase Branch

```bash
git checkout learning_phase_tasks
```

### 3. Create Your Personal Branch

Create a branch with your name (use lowercase, no spaces):

```bash
# Replace 'yourname' with your actual name (e.g., john-doe, jane-smith)
git checkout -b yourname

# Example:
git checkout -b john-doe
```

### 4. Push Your Branch to Remote

```bash
git push -u origin yourname
```

### 5. Verify Your Branch

```bash
git branch
# You should see: * yourname (with asterisk indicating current branch)

git status
# Should show: On branch yourname
```

---

## Windows Setup (WSL2)

Windows users must install WSL2 to have a Linux environment.

### Step 1: Install WSL2

Open PowerShell as Administrator and run:

```powershell
wsl --install
```

This installs WSL2 with Ubuntu by default. **Restart your computer** after installation.

### Step 2: Set Up Ubuntu

1. After restart, Ubuntu will launch automatically
2. Create a username and password (remember these!)
3. Update the system:

```bash
sudo apt update && sudo apt upgrade -y
```

### Step 3: Install Windows Terminal (Recommended)

Download from Microsoft Store: [Windows Terminal](https://aka.ms/terminal)

This provides a better terminal experience than the default Ubuntu window.

### Step 4: Access WSL2 from VS Code (Optional but Recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the "WSL" extension in VS Code
3. Open VS Code and press `Ctrl+Shift+P`
4. Type "WSL: Connect to WSL" and press Enter
5. This opens VS Code connected to your Linux environment

### Step 5: Set Up Git in WSL2

```bash
# Configure Git
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Generate SSH key for GitHub (optional but recommended)
ssh-keygen -t ed25519 -C "your.email@example.com"
# Press Enter for all prompts to use defaults

# Display your public key
cat ~/.ssh/id_ed25519.pub
# Copy this and add to GitHub: Settings → SSH and GPG keys → New SSH key
```

### WSL2 File System Notes

- Your Windows files are at: `/mnt/c/Users/YourUsername/`
- Your Linux home is at: `~` or `/home/yourusername/`
- **Best Practice**: Clone repositories in the Linux file system (`~`) for better performance
- Access WSL files from Windows: `\\wsl$\Ubuntu\home\yourusername\`

---

## macOS Setup

macOS is Unix-based, so setup is straightforward.

### Step 1: Install Homebrew (Package Manager)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the on-screen instructions to add Homebrew to your PATH.

### Step 2: Install Command Line Tools

```bash
xcode-select --install
```

Click "Install" when prompted.

### Step 3: Set Up Git

```bash
# Configure Git
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Generate SSH key for GitHub (optional but recommended)
ssh-keygen -t ed25519 -C "your.email@example.com"
# Press Enter for all prompts to use defaults

# Display your public key
cat ~/.ssh/id_ed25519.pub
# Copy this and add to GitHub: Settings → SSH and GPG keys → New SSH key
```

### macOS Notes

- Default shell is `zsh` (similar to bash)
- Use Terminal.app or install [iTerm2](https://iterm2.com/) for better experience
- File paths use forward slashes like Linux: `/Users/yourname/`

---

## Linux Setup

Most Linux distributions come ready for development.

### Step 1: Update System

**Ubuntu/Debian:**
```bash
sudo apt update && sudo apt upgrade -y
```

**Fedora:**
```bash
sudo dnf update -y
```

**Arch:**
```bash
sudo pacman -Syu
```

### Step 2: Set Up Git

```bash
# Configure Git
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Generate SSH key for GitHub (optional but recommended)
ssh-keygen -t ed25519 -C "your.email@example.com"
# Press Enter for all prompts to use defaults

# Display your public key
cat ~/.ssh/id_ed25519.pub
# Copy this and add to GitHub: Settings → SSH and GPG keys → New SSH key
```

---

## Common Tools Installation

After setting up your platform-specific environment, install these essential tools:

### 1. C++ Compiler and Build Tools

**Ubuntu/Debian/WSL2:**
```bash
sudo apt install -y build-essential g++ cmake gdb
```

**macOS:**
```bash
# Already installed with Command Line Tools
# Optionally install latest GCC:
brew install gcc cmake
```

**Fedora:**
```bash
sudo dnf install -y gcc-c++ make cmake gdb
```

**Arch:**
```bash
sudo pacman -S base-devel gcc cmake gdb
```

### 2. Performance Profiling Tools

**Ubuntu/Debian/WSL2:**
```bash
sudo apt install -y linux-tools-common linux-tools-generic
# For perf on WSL2, you may need: sudo apt install linux-tools-5.15.0-*-generic
```

**macOS:**
```bash
# macOS uses Instruments (comes with Xcode)
# For Linux perf-like tool:
brew install valgrind
```

**Fedora:**
```bash
sudo dnf install -y perf
```

**Arch:**
```bash
sudo pacman -S perf
```

### 3. Additional Useful Tools

```bash
# Ubuntu/Debian/WSL2
sudo apt install -y git vim nano htop tree

# macOS
brew install git vim htop tree

# Fedora
sudo dnf install -y git vim nano htop tree

# Arch
sudo pacman -S git vim nano htop tree
```

---

## Verify Your Setup

Run these commands to verify everything is installed correctly:

### 1. Check Compiler

```bash
g++ --version
# Should show: g++ (GCC) 9.0 or higher
```

### 2. Check CMake (Optional)

```bash
cmake --version
# Should show: cmake version 3.10 or higher
```

### 3. Check Perf (Linux/WSL2)

```bash
perf --version
# Should show: perf version
```

### 4. Check Git

```bash
git --version
# Should show: git version 2.x or higher
```

### 5. Test C++ Compilation

Create a test file:

```bash
cat > test.cpp << 'EOF'
#include <iostream>
#include <thread>

int main() {
    std::cout << "C++11 threads: ";
    std::cout << std::thread::hardware_concurrency() << " cores\n";
    return 0;
}
EOF
```

Compile and run:

```bash
g++ -std=c++17 -pthread test.cpp -o test
./test
# Should print number of CPU cores
```

If successful, clean up:

```bash
rm test.cpp test
```

---

## Workflow Guidelines

### Daily Workflow

1. **Pull latest changes** (in case updates were made):
   ```bash
   git checkout learning_phase_tasks
   git pull origin learning_phase_tasks
   git checkout yourname
   git merge learning_phase_tasks
   ```

2. **Create directory for each question**:
   ```bash
   mkdir -p Q1
   cd Q1
   mkdir screenshots
   ```

3. **Write your code**:
   ```bash
   vim main.cpp
   # or use your preferred editor
   ```

4. **Compile and test**:
   ```bash
   g++ -std=c++17 -pthread -O3 main.cpp -o program
   ./program
   ```

5. **Take screenshots** (save to `screenshots/` directory)

6. **Commit your work**:
   ```bash
   git add .
   git commit -m "Completed Q1: Race condition demonstration"
   git push origin yourname
   ```

### Compilation Best Practices

Always compile with these flags:

```bash
# For development (with debugging symbols)
g++ -std=c++17 -pthread -g -O0 -Wall -Wextra main.cpp -o program_debug

# For performance testing
g++ -std=c++17 -pthread -O3 -march=native main.cpp -o program_optimized

# With sanitizers (to catch bugs)
g++ -std=c++17 -pthread -g -O0 -fsanitize=thread main.cpp -o program_tsan
```

### Commit Message Format

Use clear, descriptive commit messages:

```bash
git commit -m "Q1: Implemented race condition with mutex synchronization"
git commit -m "Q2: Added thread pool implementation with 4 workers"
git commit -m "Q3: Completed reader-writer locks with perf analysis"
```

### Pushing Your Work

Push regularly to back up your work:

```bash
git push origin yourname
```

### Viewing Your Progress

```bash
# See your commits
git log --oneline

# See changed files
git status

# See differences
git diff
```

---

## Troubleshooting

### WSL2 Issues

**Problem**: `perf` doesn't work in WSL2
```bash
# Install WSL2 kernel with perf support
sudo apt install linux-tools-generic
```

**Problem**: File permissions issues between Windows and WSL2
```bash
# Work in WSL2 file system (~/) instead of /mnt/c/
```

**Problem**: Slow performance
```bash
# Ensure you're in WSL2 file system, not Windows mounted drives
pwd
# Should show /home/yourname, NOT /mnt/c/
```

### macOS Issues

**Problem**: `perf` not available
```bash
# Use Xcode Instruments instead, or install valgrind:
brew install valgrind
```

**Problem**: Compiler not found
```bash
# Install Command Line Tools
xcode-select --install
```

### Linux Issues

**Problem**: Permission denied for `perf`
```bash
# Temporarily allow perf for all users
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid
```

**Problem**: Thread library not found
```bash
# Make sure to compile with -pthread flag
g++ -std=c++17 -pthread main.cpp -o program
```

---

## Getting Help

If you encounter issues:

1. Check this setup guide first
2. Search for the error message online
3. Ask on the team communication channel
4. Provide:
   - Your OS (Windows/WSL2, macOS, Linux)
   - The command you ran
   - The complete error message
   - Output of `g++ --version` and `git --version`

---

## Summary Checklist

Before starting the tasks, ensure:

- [ ] WSL2 installed (Windows users only)
- [ ] Git configured with your name and email
- [ ] Repository cloned
- [ ] Personal branch created and pushed
- [ ] C++ compiler (g++) installed and working
- [ ] Can compile with `-std=c++17 -pthread` flags
- [ ] `perf` tools installed (Linux/WSL2) or alternative profiler (macOS)
- [ ] Test program compiles and runs successfully

**You're now ready to start the tasks!** Navigate to [README.md](README.md) to begin.
