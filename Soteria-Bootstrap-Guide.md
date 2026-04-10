## Soteria Bootstrap Guide

##### Hint: This is a draft version which we will improve it with time with more detailed instructions

Important: **always back up wallet.dat** to multiple secure locations before making any changes. Do NOT delete wallet.dat if it contains any balance you still use.

---

## Safe shutdown before modifying data
- **Close the Qt wallet GUI** and wait until it fully exits.
- **Preferred:** run `soteria-cli stop` (or `soteria-cli -datadir=<path> stop`) and wait for the daemon to exit — this cleanly flushes and closes databases.
- If the CLI is unresponsive, only then use `pkill soteriad` (Linux/macOS) or stop the process in Task Manager / `Stop-Process -Name soteriad` (Windows).
- Confirm shutdown before changing files: verify no soteriad process is running.
- Reason: forcing shutdown while the node is writing to disk can cause database corruption and require reindex/rescan operations.

---

## Files and folders to delete (from data directory)
Delete these folders (if present):
- assets
- chainstate
- blocks
- smartcontracts
- messages
- myrestricted
- rewards
- database

Delete these files (if present):
- *.lock (e.g., db.lock)
- db.log
- debug.log

Keep these files (do NOT delete):
- banlist.dat
- fee_estimates.dat
- mempool.dat
- peers.dat
- powcache.dat
- soteria.conf
- wallet.dat

---

## Extract bootstrap-date.zip
1. After safely stopping the daemon, extract `bootstrap-20260408.zip` into the data directory.
2. Place the new `blocks` and `chainstate` in your Soteria data directory.
   - Do NOT overwrite `wallet.dat` as this will lead to lose all your balance forever.

---

## Post-extraction checks and recovery
- Verify filesystem permissions so soteriad can read/write the datadir.
- Start the daemon and monitor logs.
- Check available disk space before extracting.
- Verify bootstrap integrity using provided checksums.

---

## Platform-specific instructions

### Linux x64
Default datadir: ~/.soteria (or check `soteria.conf` for `datadir`)
Commands:
```bash
# Safe stop
soteria-cli stop || pkill soteriad
# Backup wallet
cp ~/.soteria/wallet.dat ~/wallet-backups/wallet.dat.backup
# Remove old data
rm -rf ~/.soteria/blocks ~/.soteria/chainstate ~/.soteria/assets ~/.soteria/smartcontracts ~/.soteria/messages ~/.soteria/myrestricted ~/.soteria/rewards ~/.soteria/database
rm -f ~/.soteria/*.lock ~/.soteria/db.log ~/.soteria/debug.log
# Extract
unzip bootstrap-20260408.zip -d ~/.soteria
# Permissions
chown -R $USER:$USER ~/.soteria
chmod -R 700 ~/.soteria
# Start
soteriad
```

### Windows x64
Default datadir: %APPDATA%\Soteria
PowerShell example:
```powershell
# Safe stop
& "C:\Path\To\soteria-cli.exe" -datadir "$env:APPDATA\Soteria" stop -ErrorAction SilentlyContinue
Stop-Process -Name soteriad -ErrorAction SilentlyContinue
# Backup wallet
$datadir="$env:APPDATA\Soteria"
Copy-Item "$datadir\wallet.dat" "D:\wallet-backups\wallet.dat.backup"
# Remove old data (examples)
Remove-Item "$datadir\blocks" -Recurse -Force
Remove-Item "$datadir\chainstate" -Recurse -Force
Remove-Item "$datadir\assets","$datadir\messages","$datadir\rewards","$datadir\smartcontracts" -Recurse -Force
Remove-Item "$datadir\*.lock","$datadir\db.log","$datadir\debug.log" -Force
# Extract
Expand-Archive -Path C:\path\to\bootstrap-20260408.zip -DestinationPath $datadir -Force
# Start
Start-Process "C:\Path\To\Soteria\soteriad.exe"
```

### macOS (Intel and Apple Silicon / aarch64)
Default datadir: ~/Library/Application Support/Soteria
Commands:
```bash
# Safe stop
soteria-cli stop || pkill soteriad
DATADIR="$HOME/Library/Application Support/Soteria"
# Backup wallet
cp "$DATADIR/wallet.dat" ~/wallet-backups/wallet.dat.backup
# Remove old data
rm -rf "$DATADIR/blocks" "$DATADIR/chainstate" "$DATADIR/assets" "$DATADIR/smartcontracts" "$DATADIR/messages" "$DATADIR/rewards" "$DATADIR/database"
rm -f "$DATADIR"/*.lock "$DATADIR/db.log" "$DATADIR/debug.log"
# Extract
unzip bootstrap-20260408.zip -d "$DATADIR"
# Start (use native arm64 binary on Apple Silicon when available)
soteriad
```

### ARM / aarch / Raspberry Pi (Linux)
Same datadir rules as Linux. Ensure soteriad binary matches architecture. Commands similar to Linux example.

---

## Useful daemon flags
- `-reindex` — rebuild blockchain index from blocks (use if corruption or mismatch).
- `-rescan` — rescan wallet transactions after restoring wallet.dat.
- `-datadir=<path>` — specify custom data directory.

---
