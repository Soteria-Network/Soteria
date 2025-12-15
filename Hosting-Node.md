
# 📘 Soteria Full Node Setup Guide (VPS)

This guide explains how to deploy and secure a **full Soteria node** on a VPS. A full node validates transactions and blocks, supports the Soteria network, and can serve mining pools, wallets, or infrastructure services.

---

## 1. 🖥️ Prerequisites
- **VPS provider**: Any Linux VPS (Ubuntu/Debian recommended).  
- **Specs**: Minimum 2 CPU cores, 4 GB RAM, 40 GB SSD storage.  
- **OS**: Ubuntu/Debian.  
- **Access**: SSH root or sudo privileges.  
- **Firewall**: Ability to open/close ports.  
- **Network**: Public IPv4 address (IPv6 optional).

💡 Some VPS providers allow crypto payments. Check provider policies before purchase.

---

## 2. 🔧 System Preparation
Update your system:
```bash
sudo apt update && sudo apt upgrade -y
```

---

## 3. 📥 Install Soteria Core
Download and install binaries:
```bash
wget https://github.com/Soteria-Network/Soteria/releases/download/v1.0.9/soteria-daemon-linux64.zip
unzip soteria-daemon-linux64.zip
sudo mv soteriad/bin/* /usr/local/bin/
```

---

## 4. ⚙️ Configuration
Create the Soteria data directory and config file:
```bash
mkdir -p ~/.soteria
nano ~/.soteria/soteria.conf
```

**Example `soteria.conf`:**
```ini
# --- Basic Node Settings ---
server=1
daemon=1
listen=1
shrinkdebugfile=1
deprecatedrpc=accounts 

# --- RPC Settings ---
rpcuser=yourStrongUser
rpcpassword=yourVeryStrongPassword123!
rpcport=7896
rpcbind=0.0.0.0
rpcallowip=0.0.0.0/0   # ⚠️ Worldwide miners; secure with firewall + auth

# --- P2P Settings ---
port=8323
maxconnections=128

# --- External IP (optional) ---
externalip=203.0.113.45 // IP4
externalip=[2001:db8::1234]  // IP6

# --- Logging ---
logtimestamps=1
```

### 🔑 Notes on `externalip`
- Tells your node what public IP to advertise.  
- Usually auto-detected, but set manually if behind VPN/NAT or to support IPv6.  
- Example:
  ```ini
  externalip=203.0.113.45
  externalip=[2001:db8::1234]
  ```

---

## 5. 🚀 Running the Node
Start node:
```bash
soteriad -daemon
```

Check logs:
```bash
tail -f ~/.soteria/debug.log
```

Stop node:
```bash
soteria-cli stop
```

---

## 6. 🔒 Security Hardening
### Firewall (UFW)
```bash
sudo apt install ufw 
sudo ufw allow 22/tcp    # SSH
sudo ufw allow 8323/tcp  # Soteria P2P
sudo ufw allow 7896/tcp  # Soteria RPC
sudo ufw allow 443 # optional only in case you wanted to use tls
sudo ufw enable
sudo ufw status // To check status
```

### SSH Security
- Disable root login (`/etc/ssh/sshd_config` → `PermitRootLogin no`).  
- Use SSH keys instead of passwords.  

### Fail2Ban
```bash
sudo apt install fail2ban -y
```

### Regular Updates
```bash
sudo apt update && sudo apt upgrade -y
```

### Monitoring Tools
Use `htop`, `vnstat`, or `netstat` to watch resource usage.

---

## 7. 📊 Verification
Check sync status:
```bash
soteria-cli getblockchaininfo
```

Check peer connections:
```bash
soteria-cli getpeerinfo
```

---

## 8. 🛡️ Maintenance
- Backup `wallet.dat` if using wallet functionality.  
- Monitor logs (`~/.soteria/debug.log`).
- Check logs (tail -f ~/.soteria/debug.log).
- Restart node after system updates.  
- Run under a non‑root user for extra safety.

---

## 9. ⚙️ Auto‑Restart with systemd
Create a service file:
```bash
sudo nano /etc/systemd/system/soteriad.service
```

**Example:**
```ini
[Unit]
Description=Soteria Full Node
After=network.target

[Service]
ExecStart=/usr/local/bin/soteriad -conf=/home/YOURUSER/.soteria/soteria.conf -datadir=/home/YOURUSER/.soteria
ExecStop=/usr/local/bin/soteria-cli stop
Restart=always
RestartSec=10
User=YOURUSER
Group=YOURUSER

[Install]
WantedBy=multi-user.target
```

Save the file by pressing: ctrl+x then y then enter

Reload and enable:
```bash
sudo systemctl daemon-reload
sudo systemctl enable soteriad
sudo systemctl start soteriad
```

Check status:
```bash
systemctl status soteriad
```

Logs:
```bash
journalctl -u soteriad -f
```

⚒️ Next steps you might consider

    Security: instead of running as root, create a dedicated soteria user and adjust User= and Group= in the unit file.

    Autostart at boot: already enabled, but you can confirm with
    systemctl is-enabled soteriad
Monitoring: use systemctl status soteriad or journalctl to watch for warnings like “Potential stale tip detected.”

🎯 Troubleshooting

Unit file failed because:

    Wrong path (/home/username/... instead of /username/...).

    Possibly using a copied binary that doesn’t run.

    Possibly leaving -daemon in systemd context.

Fix the paths and point to the working binary (/soteriad), drop -daemon, and systemd will keep it alive.
---

🔎 How to diagnose

    Check the node’s logs if soteriad exits quickly, the reason should be in ~/.soteria/debug.log. Tail it right after restart:
    tail -n 50 ~/.soteria/debug.log
    
Look for errors like “invalid argument,” “cannot open database,” “conf file not found,” etc.

Verify config file path Your unit file points to.

Test manually without systemd. Run the same command systemd is using:

/username/soteriad -conf=/username/.soteria/soteria.conf -datadir=/username/.soteria

If it fails, you’ll see the error directly in your shell.


✅ How to locate the actual binary

Run one of these to find where the daemon lives:

ps -ef | grep soteriad

This will show the full path of the running process.

Or:

which soteriad

If it’s in your $PATH, this will print the location.

Or a broader search:

find / -type f -name soteriad 2>/dev/null

That will crawl the filesystem and show you the binary’s location.

⚙️ Fixing the unit file

Once you know the real path (say it’s /usr/bin/soteriad or /home/root/soteriad or another path), update your unit file:

ExecStart=/path/to/soteriad -conf=/home/root/.soteria/soteria.conf -datadir=/home/root/.soteria

ExecStop=/path/to/soteria-cli stop

Making Binaries Accessible System‑Wide

When you download binaries, they often land in your current working directory (for example, /root/ after extraction). To avoid having to cd into that folder every time, you can place them in a standard system path like /usr/local/bin. This ensures they’re available globally and can be called from anywhere.

Run:

sudo cp soteriad /usr/local/bin/

sudo cp soteria-cli /usr/local/bin/

After this step, you can launch the node or use the CLI from any directory:

## ✅ Summary
With this setup, your Soteria full node will:
- Auto‑start on boot and auto‑recover from crashes.  
- Be secured with firewall, SSH hardening, and Fail2Ban.  
- Support worldwide miners via RPC (7896) and P2P networking (8323).  
- Advertise external IPs if needed for connectivity.  

You now have a **secure, production‑ready Soteria full node** running on your VPS.

---
