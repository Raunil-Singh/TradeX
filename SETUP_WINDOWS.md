# Windows WSL2 Setup Guide

This guide provides detailed instructions for Windows users to set up WSL2 (Windows Subsystem for Linux 2) for the TradeX multi-threading tasks.

## Why WSL2?

WSL2 provides a real Linux kernel on Windows, giving you:
- Native Linux performance for development
- Access to Linux tools like `perf` for profiling
- Consistent environment with Linux and macOS users
- Better compatibility with threading and synchronization primitives

---

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Installation Steps](#installation-steps)
3. [Initial Configuration](#initial-configuration)
4. [Installing Development Tools](#installing-development-tools)
5. [IDE Integration](#ide-integration)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

---

## System Requirements

- Windows 10 version 2004 or higher (Build 19041+), or Windows 11
- 64-bit processor
- 4GB RAM minimum (8GB recommended)
- Virtualization enabled in BIOS

### Check Your Windows Version

Press `Win + R`, type `winver`, and press Enter. Ensure:
- Version 21H2 or later (Windows 10)
- Or Windows 11

---

## Installation Steps

### Method 1: Simple Installation (Recommended)

1. **Open PowerShell as Administrator**
   - Right-click Start button
   - Select "Windows PowerShell (Admin)" or "Terminal (Admin)"

2. **Install WSL2**
   ```powershell
   wsl --install
   ```

3. **Restart Your Computer**
   - This is required for the installation to complete

4. **First Launch**
   - After restart, Ubuntu will launch automatically
   - Wait for installation to complete (~5 minutes)
   - Create a username (lowercase, no spaces recommended)
   - Create a password (you won't see characters as you type - this is normal)
   - Confirm password

### Method 2: Manual Installation (If Method 1 Fails)

1. **Enable WSL Feature**
   ```powershell
   dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
   ```

2. **Enable Virtual Machine Platform**
   ```powershell
   dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart
   ```

3. **Restart Your Computer**

4. **Download WSL2 Kernel Update**
   - Visit: https://aka.ms/wsl2kernel
   - Download and run the installer

5. **Set WSL2 as Default**
   ```powershell
   wsl --set-default-version 2
   ```

6. **Install Ubuntu**
   - Open Microsoft Store
   - Search for "Ubuntu"
   - Install "Ubuntu" (latest version)
   - Launch Ubuntu from Start menu
   - Create username and password

---

## Initial Configuration

### 1. Update Ubuntu

After first login, update the system:

```bash
sudo apt update
sudo apt upgrade -y
```

This may take 5-10 minutes.

### 2. Verify WSL2 Version

In PowerShell:

```powershell
wsl --list --verbose
```

You should see:
```
  NAME      STATE           VERSION
* Ubuntu    Running         2
```

If VERSION shows 1, upgrade to WSL2:

```powershell
wsl --set-version Ubuntu 2
```

### 3. Set Ubuntu as Default

```powershell
wsl --set-default Ubuntu
```

### 4. Configure Git

In Ubuntu terminal:

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

### 5. Generate SSH Key for GitHub

```bash
ssh-keygen -t ed25519 -C "your.email@example.com"
```

Press Enter for all prompts to accept defaults.

Display your public key:

```bash
cat ~/.ssh/id_ed25519.pub
```

Copy this entire output and add to GitHub:
1. Go to GitHub.com → Settings
2. Click "SSH and GPG keys"
3. Click "New SSH key"
4. Paste the key and save

---

## Installing Development Tools

### Essential Tools

```bash
sudo apt install -y build-essential g++ cmake gdb git vim nano htop tree
```

### Performance Profiling Tools

```bash
sudo apt install -y linux-tools-common linux-tools-generic
```

If `perf` doesn't work, install kernel-specific tools:

```bash
# Find your kernel version
uname -r

# Install matching tools (replace version if different)
sudo apt install -y linux-tools-5.15.0-*-generic
```

### Verify Installation

```bash
g++ --version
# Should show: g++ (Ubuntu ...) 9.4.0 or higher

git --version
# Should show: git version 2.x

cmake --version
# Should show: cmake version 3.x

perf --version
# Should show: perf version 5.x
```

---

## IDE Integration

### Option 1: Visual Studio Code with WSL Extension (Recommended)

1. **Install VS Code on Windows**
   - Download from: https://code.visualstudio.com/
   - Run installer

2. **Install WSL Extension**
   - Open VS Code
   - Click Extensions icon (or press `Ctrl+Shift+X`)
   - Search for "WSL"
   - Install "WSL" extension by Microsoft

3. **Connect to WSL**
   - Press `Ctrl+Shift+P`
   - Type "WSL: Connect to WSL"
   - Or click the green icon in bottom-left corner
   - Select "Connect to WSL"

4. **Open Project**
   - In WSL-connected VS Code, press `Ctrl+O`
   - Navigate to `/home/yourusername/TradeX`
   - Open the folder

5. **Install C++ Extension in WSL**
   - Click Extensions icon
   - Search for "C/C++"
   - Install "C/C++" extension by Microsoft
   - Install "CMake Tools" extension

6. **Configure IntelliSense**
   - Open any `.cpp` file
   - Click "Select IntelliSense Configuration" if prompted
   - Choose "g++"

### Option 2: CLion

1. **Install CLion on Windows**
   - Download from: https://www.jetbrains.com/clion/
   - Student license available at: https://www.jetbrains.com/student/

2. **Configure WSL Toolchain**
   - File → Settings → Build, Execution, Deployment → Toolchains
   - Click "+"
   - Select "WSL"
   - CLion will auto-detect WSL Ubuntu

3. **Open Project**
   - File → Open
   - Navigate to `\\wsl$\Ubuntu\home\yourusername\TradeX`

### Option 3: Terminal-Based (vim/nano)

Simple and lightweight:

```bash
cd ~/TradeX
vim main.cpp
# or
nano main.cpp
```

---

## Best Practices

### File System Guidelines

**Always work in WSL file system for best performance:**

```bash
# Good: Work in your Linux home directory
cd ~
cd ~/TradeX

# Bad: Work in Windows file system (slow)
cd /mnt/c/Users/YourName/Documents
```

### Accessing Files

**From WSL → Windows:**
```bash
# Windows C: drive is at
/mnt/c/

# Example: Access Windows Desktop
cd /mnt/c/Users/YourName/Desktop
```

**From Windows → WSL:**
```
# In File Explorer, type in address bar:
\\wsl$\Ubuntu\home\yourusername\TradeX

# Or use network path:
\\wsl$\Ubuntu\
```

### Performance Tips

1. **Keep projects in WSL file system** (`~/TradeX` not `/mnt/c/...`)
2. **Use WSL2 (not WSL1)** for better I/O performance
3. **Don't mix Windows and Linux Git** - use Git from WSL only
4. **Allocate more memory to WSL** if needed (see Advanced Configuration)

### Windows Terminal Setup (Recommended)

1. **Install Windows Terminal**
   - Microsoft Store → Search "Windows Terminal"
   - Or download from: https://aka.ms/terminal

2. **Set Ubuntu as Default**
   - Open Windows Terminal
   - Settings (Ctrl+,)
   - Startup → Default profile → Select "Ubuntu"

3. **Customize Appearance**
   - Profiles → Ubuntu → Appearance
   - Choose font, color scheme, transparency

---

## Advanced Configuration

### Increase WSL2 Memory Limit

Create `.wslconfig` in Windows home directory:

1. Open PowerShell:
   ```powershell
   notepad $env:USERPROFILE\.wslconfig
   ```

2. Add configuration:
   ```ini
   [wsl2]
   memory=8GB
   processors=4
   swap=4GB
   ```

3. Restart WSL:
   ```powershell
   wsl --shutdown
   wsl
   ```

### Enable systemd (Optional)

Edit `/etc/wsl.conf` in Ubuntu:

```bash
sudo nano /etc/wsl.conf
```

Add:
```ini
[boot]
systemd=true
```

Restart WSL:
```powershell
wsl --shutdown
wsl
```

---

## Troubleshooting

### WSL Won't Install

**Error: "WSL 2 requires an update to its kernel component"**

Solution:
1. Download kernel update: https://aka.ms/wsl2kernel
2. Install the update
3. Try `wsl --install` again

**Error: "Virtualization is not enabled"**

Solution:
1. Restart computer
2. Enter BIOS (usually F2, F10, or Del during startup)
3. Find "Virtualization" or "Intel VT-x" or "AMD-V"
4. Enable it
5. Save and exit BIOS

### Ubuntu Won't Start

**Error: "The attempted operation is not supported"**

Solution:
```powershell
# Set WSL2 as default
wsl --set-default-version 2

# Convert existing installation
wsl --set-version Ubuntu 2
```

### Performance Issues

**Compilation is slow**

Solution:
- Ensure you're working in WSL file system (`~/`), not `/mnt/c/`
- Check WSL version: `wsl --list --verbose` (should be version 2)

### Perf Not Working

**Error: "perf: No such file or directory"**

Solution:
```bash
# Install kernel-specific tools
sudo apt install -y linux-tools-$(uname -r)
```

**Error: "Permission denied"**

Solution:
```bash
# Allow perf for all users
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid
```

### Git Authentication Issues

**Error: "Permission denied (publickey)"**

Solution:
1. Verify SSH key exists:
   ```bash
   ls ~/.ssh/
   # Should show id_ed25519 and id_ed25519.pub
   ```

2. If missing, generate new key:
   ```bash
   ssh-keygen -t ed25519 -C "your.email@example.com"
   ```

3. Add to GitHub (copy output):
   ```bash
   cat ~/.ssh/id_ed25519.pub
   ```

4. Test connection:
   ```bash
   ssh -T git@github.com
   # Should say: "Hi username! You've successfully authenticated"
   ```

### Can't Find Files

**Can't see Windows files in WSL**

Solution:
```bash
# Windows drives are mounted under /mnt/
ls /mnt/c/Users/
```

**Can't find WSL files in Windows**

Solution:
- Open File Explorer
- Type in address bar: `\\wsl$\Ubuntu\home\yourusername`

---

## Useful Commands

### WSL Management (PowerShell)

```powershell
# List installed distributions
wsl --list --verbose

# Shutdown WSL
wsl --shutdown

# Start specific distribution
wsl -d Ubuntu

# Uninstall distribution (WARNING: Deletes all data)
wsl --unregister Ubuntu
```

### Inside WSL

```bash
# Check disk usage
df -h

# Update all packages
sudo apt update && sudo apt upgrade -y

# Clean package cache
sudo apt autoremove -y
sudo apt clean

# Check WSL version from inside
cat /proc/version
```

---

## Quick Reference

### Common Paths

| Description | Windows Path | WSL Path |
|------------|--------------|----------|
| Windows C: drive | `C:\` | `/mnt/c/` |
| Windows Desktop | `C:\Users\Name\Desktop` | `/mnt/c/Users/Name/Desktop` |
| WSL Home | `\\wsl$\Ubuntu\home\username` | `~` or `/home/username` |
| Windows User | `C:\Users\Name` | `/mnt/c/Users/Name` |

### Essential Commands

| Task | Command |
|------|---------|
| Open Ubuntu | Type `wsl` in PowerShell or Start Menu |
| Exit WSL | `exit` or `Ctrl+D` |
| Shutdown WSL | `wsl --shutdown` (in PowerShell) |
| Update packages | `sudo apt update && sudo apt upgrade` |
| Install package | `sudo apt install package-name` |

---

## Next Steps

Once setup is complete:

1. ✅ Verify all tools installed
2. ✅ Clone TradeX repository
3. ✅ Create your personal branch
4. ✅ Complete test compilation
5. 🚀 Start with Q0 (perf profiling)

Return to [SETUP.md](SETUP.md) for workflow guidelines.
