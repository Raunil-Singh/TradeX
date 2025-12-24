# Development Environment Setup

This guide will help you set up a consistent development environment across Windows, macOS, and Linux for completing the multi-threading tasks.

## Prerequisites

All mentees should work in a Unix-like environment to ensure consistency. This means:
- **Linux users**: Use your native environment
- **macOS users**: Use your native terminal (macOS is Unix-based)
- **Windows users**: Set up WSL2 (Windows Subsystem for Linux) - **see detailed section below**

---

## Table of Contents

1. [Git Repository Setup](#git-repository-setup)
2. [Windows Setup (WSL2) - DETAILED](#windows-setup-wsl2---detailed)
3. [macOS Setup](#macos-setup)
4. [Linux Setup](#linux-setup)
5. [Common Tools Installation](#common-tools-installation)
6. [IDE Integration](#ide-integration)
7. [Demo Programs](#demo-programs)
8. [Verify Your Setup](#verify-your-setup)
9. [Workflow Guidelines](#workflow-guidelines)
10. [Troubleshooting](#troubleshooting)

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

## Windows Setup (WSL2) - DETAILED

Windows users must install WSL2 to have a Linux environment with native performance and access to Linux tools like `perf`.

### System Requirements

- Windows 10 version 2004+ (Build 19041+) or Windows 11
- 64-bit processor with virtualization enabled
- 4GB RAM minimum (8GB recommended)

**Check Version**: Press `Win + R`, type `winver`, press Enter.

### Quick Installation

1. **Open PowerShell as Administrator** (Right-click Start → "Terminal (Admin)")

2. **Install WSL2**
   ```powershell
   wsl --install
   ```

3. **Restart Computer**

4. **First Launch** (Ubuntu auto-starts after restart)
   - Create username (lowercase, no spaces)
   - Create password (won't display characters)
   - Update system:
     ```bash
     sudo apt update && sudo apt upgrade -y
     ```

### Verify Installation

In PowerShell:
```powershell
wsl --list --verbose
```

Should show `VERSION` as `2`. If it shows `1`:
```powershell
wsl --set-version Ubuntu 2
```

### Important: File System Usage

**Always work in WSL file system for performance:**

```bash
# ✅ Good: Linux home directory
cd ~/TradeX

# ❌ Bad: Windows file system (10x slower)
cd /mnt/c/Users/YourName/Documents
```

**Accessing Files:**
- **WSL → Windows**: `/mnt/c/` (Windows C: drive)
- **Windows → WSL**: File Explorer → `\\wsl$\Ubuntu\home\yourusername\`

### Recommended: Windows Terminal

Install from Microsoft Store for better experience:
- Settings → Startup → Default profile → "Ubuntu"

### Common Issues

**"WSL 2 requires kernel update"**: Download from https://aka.ms/wsl2kernel

**"Virtualization not enabled"**: Enable in BIOS (F2/F10/Del on startup)

**Slow compilation**: Ensure you're in `~/` not `/mnt/c/`

**Perf not working**:
```bash
sudo apt install -y linux-tools-$(uname -r)
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid
```

### Useful Commands

```powershell
# PowerShell
wsl --shutdown          # Restart WSL
wsl --list --verbose    # Check status
```

```bash
# Inside WSL
sudo apt update && sudo apt upgrade -y  # Update packages
df -h                                     # Check disk space
```

After setup, continue to [Git Repository Setup](#git-repository-setup).

---

## macOS Setup - DETAILED

macOS is Unix-based with native development tools, making it ideal for C++ multi-threading work.

### System Requirements

- macOS 10.15 (Catalina) or newer
- At least 4GB free disk space
- Administrator access

**Check Version**: Apple menu → About This Mac

### Step 1: Install Command Line Tools

This installs essential compilers and tools:

```bash
xcode-select --install
```

Click "Install" when prompted (downloads ~1-2GB, takes 10-15 minutes).

**Verify installation:**
```bash
xcode-select -p
# Should show: /Library/Developer/CommandLineTools

g++ --version
# Should show: Apple clang version 14.0 or higher
```

### Step 2: Install Homebrew (Package Manager)

Homebrew simplifies installing additional tools:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

**Important**: Follow the on-screen "Next steps" to add Homebrew to PATH.

For Apple Silicon (M1/M2/M3), add to `~/.zprofile`:
```bash
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"
```

For Intel Macs, it's added automatically to `/usr/local/bin`.

**Verify installation:**
```bash
brew --version
# Should show: Homebrew 4.x or higher
```

### Step 3: Install Additional Development Tools

```bash
# Install GNU GCC (alternative to Clang)
brew install gcc

# Install CMake and other build tools
brew install cmake gdb

# Install profiling and analysis tools
brew install valgrind htop

# Install better terminal (optional)
brew install --cask iterm2
```

**Note**: `gdb` on macOS requires code signing. For most tasks, `lldb` (built-in) works fine.

### Step 4: Configure Git

```bash
# Set your identity
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Generate SSH key for GitHub
ssh-keygen -t ed25519 -C "your.email@example.com"
# Press Enter for all prompts (uses default location ~/.ssh/id_ed25519)

# Start SSH agent
eval "$(ssh-agent -s)"

# Add SSH key to agent
ssh-add ~/.ssh/id_ed25519

# Copy public key to clipboard
pbcopy < ~/.ssh/id_ed25519.pub
# Now paste this into GitHub: Settings → SSH and GPG keys → New SSH key

# Test GitHub connection
ssh -T git@github.com
# Should say: "Hi username! You've successfully authenticated"
```

### macOS Development Notes

**Shell**: Default is `zsh` (similar to bash)
- Config file: `~/.zshrc` (like `~/.bashrc` in Linux)
- Most bash commands work identically

**File System**:
- Home directory: `/Users/yourname/` or `~`
- Case-insensitive by default (but case-preserving)
- Use forward slashes for paths

**Terminal Options**:
- **Terminal.app**: Built-in, works well
- **iTerm2**: Feature-rich alternative (split panes, search, themes)

**Compiler Notes**:
- Default `g++` is actually Clang (Apple's compiler)
- For GNU GCC, use `g++-13` or `gcc-13` (version from Homebrew)
- Both work for the curriculum

### Profiling on macOS

Since `perf` is Linux-specific, use these alternatives:

1. **Instruments** (GUI, comes with Xcode):
   ```bash
   # Install Xcode (optional, large download)
   xcode-select --install
   ```
   Launch: Applications → Xcode → Open Developer Tool → Instruments

2. **Activity Monitor** (Built-in):
   - Applications → Utilities → Activity Monitor
   - View CPU, memory, threads in real-time

3. **time command** (Built-in):
   ```bash
   time ./your_program
   ```

4. **Valgrind** (Installed via Homebrew):
   ```bash
   valgrind --tool=callgrind ./your_program
   ```

### Common Issues

**"xcode-select: error: command line tools are already installed"**
- Already set up! Verify with `g++ --version`

**"command not found: brew"**
- Add to PATH (see Homebrew installation output)
- Restart terminal after adding to PATH

**"Permission denied" when using gdb**
- Use `lldb` instead: `lldb ./program`
- Or code-sign gdb (complex, usually unnecessary)

**Compilation errors with pthread**
- macOS Clang handles threading automatically
- Still use `-pthread` flag for compatibility

### Useful Commands

```bash
# Check architecture (Intel vs Apple Silicon)
uname -m
# x86_64 = Intel, arm64 = Apple Silicon

# Update Homebrew packages
brew update && brew upgrade

# View CPU info
sysctl -n machdep.cpu.brand_string
sysctl -n hw.ncpu  # Number of CPUs

# Monitor processes
top  # or: htop (if installed)
```

After setup, continue to [Git Repository Setup](#git-repository-setup).

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

## IDE Integration

Choose your preferred development environment. All options work across platforms.

### Option 1: Visual Studio Code (Recommended for All Platforms)

#### Windows (WSL2)

1. **Install VS Code on Windows**: https://code.visualstudio.com/

2. **Install WSL Extension**
   - Open VS Code
   - Extensions (`Ctrl+Shift+X`)
   - Search "WSL"
   - Install "WSL" by Microsoft

3. **Connect to WSL**
   - Press `Ctrl+Shift+P`
   - Type "WSL: Connect to WSL"
   - Or click green icon in bottom-left → "Connect to WSL"

4. **Open Project**
   - In WSL-connected VS Code: `Ctrl+O`
   - Navigate to `/home/yourusername/TradeX`

5. **Install C++ Extensions in WSL**
   - "C/C++" by Microsoft
   - "CMake Tools"

6. **Configure IntelliSense**
   - Open any `.cpp` file
   - Select "g++" when prompted

#### macOS and Linux

1. **Install VS Code**: https://code.visualstudio.com/

2. **Install C++ Extensions**
   - "C/C++" by Microsoft
   - "CMake Tools"

3. **Open Project**
   - File → Open Folder
   - Navigate to `TradeX`

### Option 2: CLion (All Platforms)

1. **Install CLion**: https://www.jetbrains.com/clion/
   - Free for students: https://www.jetbrains.com/student/

2. **Configure Toolchain**
   - **Windows**: File → Settings → Toolchains → Add "WSL"
   - **macOS/Linux**: Auto-detected, or manually set to system GCC

3. **Open Project**
   - File → Open
   - **Windows**: Navigate to `\\wsl$\Ubuntu\home\yourusername\TradeX`
   - **macOS/Linux**: Navigate to `~/TradeX`

### Option 3: Terminal-Based Editors (vim/nano)

Lightweight and available on all platforms:

```bash
cd ~/TradeX
vim main.cpp  # or nano main.cpp
```

**Recommended for**: Quick edits, remote development, minimalist workflow

---

## Demo Programs

Before starting the tasks, explore the demo programs in the `demos/` directory to understand threading and timing concepts.

### Available Demos

1. **`thread_demo.cpp`** - Combined threading and timing
2. **`thread_basics.cpp`** - Comprehensive thread creation patterns
3. **`timing_precision.cpp`** - Deep dive into chrono timing

### Running the Demos

```bash
cd ~/TradeX/demos

# Compile all demos
g++ -std=c++17 -pthread -O2 thread_demo.cpp -o thread_demo
g++ -std=c++17 -pthread -O2 thread_basics.cpp -o thread_basics
g++ -std=c++17 -pthread -O2 timing_precision.cpp -o timing_precision

# Run them
./thread_demo
./thread_basics
./timing_precision
```

See `demos/README.md` for detailed descriptions of each demo.

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

### All Platforms

**Problem**: Thread library not found
```bash
# Always compile with -pthread flag
g++ -std=c++17 -pthread main.cpp -o program
```

**Problem**: Git authentication issues
```bash
# Verify SSH key exists
ls ~/.ssh/
# Should show id_ed25519 and id_ed25519.pub

# If missing, generate:
ssh-keygen -t ed25519 -C "your.email@example.com"

# Display and add to GitHub:
cat ~/.ssh/id_ed25519.pub

# Test connection:
ssh -T git@github.com
```

### Windows/WSL2 Specific

**Problem**: `perf` doesn't work
```bash
sudo apt install -y linux-tools-$(uname -r)
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid
```

**Problem**: File permission issues
```bash
# Work in WSL2 file system (~/) instead of /mnt/c/
pwd  # Should show /home/yourname, NOT /mnt/c/
```

**Problem**: Slow performance
```bash
# Ensure you're in WSL2 file system
# NOT in Windows mounted drives (/mnt/c/)
```

**Problem**: Can't find WSL files in Windows
- File Explorer → Address bar: `\\wsl$\Ubuntu\home\yourusername`

### macOS Specific

**Problem**: `perf` not available
```bash
# Use Xcode Instruments (comes with Xcode)
# Or install valgrind:
brew install valgrind
```

**Problem**: Compiler not found
```bash
# Install Command Line Tools
xcode-select --install
```

### Linux Specific

**Problem**: Permission denied for `perf`
```bash
# Temporarily allow perf for all users
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid

# To make permanent, add to /etc/sysctl.conf:
echo "kernel.perf_event_paranoid = -1" | sudo tee -a /etc/sysctl.conf
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

## Ready to Start

Once you've completed the setup for your platform, you should have:

**Platform-specific environment**: WSL2 (Windows), native Unix terminal (macOS/Linux) with all system updates applied.

**Git configured**: Your identity set globally, SSH key generated and added to GitHub for authentication.

**Repository ready**: TradeX cloned to `~/TradeX`, checked out to `learning_phase_tasks` branch, with your personal branch (`yourname`) created and pushed to remote.

**Development tools**: C++ compiler (g++) working with `-std=c++17 -pthread` support, profiling tools (`perf` on Linux/WSL2, Instruments/valgrind on macOS), and your preferred IDE or editor configured.

**Verification passed**: Test compilation successful, demo programs running without errors, git operations (commit, push) working correctly.

### Next Steps

1. **Explore the demos**: Run the programs in `demos/` directory to understand threading and timing concepts
2. **Start the curriculum**: Navigate to [README.md](README.md) and begin with Q0 (perf profiling prerequisite)
3. **Follow the workflow**: Use the daily workflow guidelines above for committing and tracking your progress
