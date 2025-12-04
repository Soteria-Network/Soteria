**Quick Setup version** of Soteria Full Node guide — stripped down to just the essentials so community members can get running fast:

---

# ⚡ Soteria Full Node Quick Setup (VPS)

### 1. System Prep
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install unzip ufw fail2ban -y
```

---

### 2. Install Soteria Core
```bash
wget https://github.com/Soteria-Network/Soteria/releases/download/v1.0.9/soteria-daemon-linux64.zip
unzip soteria-daemon-linux64.zip
sudo mv soteriad/bin/* /usr/local/bin/
```

---

### 3. Config File
```bash
mkdir -p ~/.soteria
nano ~/.soteria/soteria.conf
```

**Minimal `soteria.conf`:**
```ini
server=1
daemon=1
listen=1

rpcuser=yourStrongUser
rpcpassword=yourStrongPassword123!
rpcport=7896
rpcbind=0.0.0.0
rpcallowip=0.0.0.0/0   # worldwide miners

port=8323
maxconnections=128

logtimestamps=1
```

---

### 4. Firewall
```bash
sudo ufw allow 22/tcp    # SSH
sudo ufw allow 8323/tcp  # P2P
sudo ufw allow 7896/tcp  # RPC
sudo ufw enable
```

---

### 5. Run Node
```bash
soteriad -daemon
tail -f ~/.soteria/debug.log
```

Stop:
```bash
soteria-cli stop
```

---

### 6. Verify
```bash
soteria-cli getblockchaininfo
soteria-cli getpeerinfo
```

---

### 7. Auto‑Restart (systemd)
```bash
sudo nano /etc/systemd/system/soteriad.service
```

**Content:**
```ini
[Unit]
Description=Soteria Full Node
After=network.target

[Service]
ExecStart=/usr/local/bin/soteriad -daemon -conf=/home/YOURUSER/.soteria/soteria.conf -datadir=/home/YOURUSER/.soteria
ExecStop=/usr/local/bin/soteria-cli stop
Restart=always
RestartSec=10
User=YOURUSER
Group=YOURUSER

[Install]
WantedBy=multi-user.target
```

Enable:
```bash
sudo systemctl daemon-reload
sudo systemctl enable soteriad
sudo systemctl start soteriad
```

---
