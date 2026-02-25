```markdown
# Soteria RPC API Reference (Draft Version)

Complete documentation of all Soteria RPC commands, `soteria-cli` commands, integration guidelines, and error handling.  
Essential for node launch, exchanges, mining pools, and block explorers.

## Protocol Overview

- **Protocol:** JSON-RPC
- **Transport:** HTTP POST
- **RPC Authentication:** HTTP Basic Auth (username/password)
- **RPC Binding:** 127.0.0.1 (localhost only)
- **Open Relay Policy:** Any well-formed script up to 10KB is standard and relayable, maintaining consensus security through strict execution limits.

## Wallet Security

- **Backup `wallet.dat`** – store multiple secure copies.
- **Wallet Encryption Recommended** – encrypt your wallet.
- **OS-level Encryption** – another security option.

---

## Configuration (`soteria.conf`)

Create `soteria.conf` in your data directory for persistent RPC settings:

| OS      | Path                                      |
|---------|-------------------------------------------|
| Linux   | `~/.soteria/soteria.conf`                 |
| macOS   | `~/Library/Application Support/Soteria/soteria.conf` |
| Windows | `%APPDATA%\Soteria\soteria.conf`          |

### Example `soteria.conf`

```ini
# Enable RPC server
server=1

# RPC credentials (use strong passwords!)
rpcuser=username
rpcpassword=SecurePassword

# RPC settings
port=8323
rpcallowip=127.0.0.1
rpcallowip=192.168.1.0/24

# Network peers
addnode=130.255.9.108:8323
addnode=145.239.3.70:8323
addnode=149.102.156.62:8323
addnode=178.72.89.199:8323
addnode=27.254.39.27:8323
addnode=45.10.160.253:8323
addnode=45.84.107.174:8323
addnode=51.195.24.31:8323
addnode=69.42.222.20:8323
addnode=86.124.29.223:8323
addnode=89.105.213.189:8323

# Mining settings
gen=0
genproclimit=6
```

## Starting the Node

```bash
# Start daemon (uses soteria.conf)
./soteriad -daemon

# Or with command-line options
./soteriad -rpcuser=username -rpcpassword=password -server

# Execute an RPC command
./soteria-cli getinfo
```

---

## General Commands

### `help`

Lists all available RPC commands.

```bash
./soteria-cli help
```

### `getinfo`

Returns general information about the node and wallet version.  
**Returns:** balance, blocks, connections, generate, genproclimit, difficulty, protocol version.

```bash
./soteria-cli getinfo
```

### `stop`

Gracefully shuts down the Soteria node.

```bash
./soteria-cli stop
```

---

## Blockchain Commands

### `getblockcount`

Returns the total number of blocks in the longest blockchain.

```bash
./soteria-cli getblockcount
```

### `getblockhash <index>`

Returns the hash of the block at a specific height.  
**Parameters:** `index` (number)

```bash
./soteria-cli getblockhash 0
```

### `getblock <hash>`

Returns detailed information about a block given its hash.  
**Parameters:** `hash` (string)

```bash
./soteria-cli getblock "000005a8d6b5..."
```

### `getdifficulty`

Returns the current proof-of-work difficulty.

```bash
./soteria-cli getdifficulty
```

### `getbestblockhash`

Returns the hash of the best (tip) block in the longest chain.

```bash
./soteria-cli getbestblockhash
```

### `getblockheader <hash>`

Returns block header information without full transaction data.  
**Parameters:** `hash` (string)

```bash
./soteria-cli getblockheader "000005a8d6b5..."
```

---

## SPV Commands

### `gettxoutproof <txids> [blockhash]`

Generate Merkle proof that transactions are in a block.  
**Parameters:**
- `txids` (JSON array)
- `blockhash` (string, optional)

```bash
./soteria-cli gettxoutproof '["txid1","txid2"]'
./soteria-cli gettxoutproof '["txid1"]' "blockhash"
```

### `verifytxoutproof <proof>`

Verify Merkle proof and return transaction IDs.  
**Parameters:** `proof` (string) – hex-encoded proof

```bash
./soteria-cli verifytxoutproof "hex_proof_data"
```

---

## Wallet Commands

### `getbalance`

Returns the total available balance in the wallet.

```bash
./soteria-cli getbalance
```

### `getnewaddress [label]`

Generates a new Soteria address for receiving payments.  
**Parameters:** `label` (string, optional)

```bash
./soteria-cli getnewaddress
./soteria-cli getnewaddress "soteria_address"
```

### `sendtoaddress <soteria_address> <amount>`

Sends SOTER to a specified address.  
**Parameters:** `address` (string), `amount` (number)

```bash
./soteria-cli sendtoaddress Safr5Vc9UeuH3BzYzwuAjRKJtsinSUFysD 5
```

### `listtransactions [count] [from]`

Returns list of recent transactions in the wallet.  
**Parameters:** `count` (number, default 10), `from` (number, default 0)

```bash
./soteria-cli listtransactions
./soteria-cli listtransactions 20 10
```

### `listunspent [minconf] [maxconf]`

Returns array of unspent transaction outputs (UTXOs).  
**Parameters:** `minconf` (number, default 1), `maxconf` (number, default 999999)

```bash
./soteria-cli listunspent
./soteria-cli listunspent 5 999999
```

### `dumpprivkey <soteria_address>`

Export private key for an address in WIF format.  
**Parameters:** `address` (string)

```bash
./soteria-cli dumpprivkey Safr5Vc9UeuH3BzYzwuAjRKJtsinSUFysD
```

### `importprivkey <wif> [label]`

Import private key and trigger blockchain rescan.  
**Parameters:** `wif` (string), `label` (string, optional)

```bash
./soteria-cli importprivkey <WIF> "label"
```

---

## Transaction Commands

### `gettransaction <txid>`

Get detailed information about a wallet transaction.  
**Parameters:** `txid` (string)

```bash
./soteria-cli gettransaction abc123...
```

### `getrawtransaction <txid> [verbose]`

Get raw transaction data (hex or JSON).  
**Parameters:** `txid` (string), `verbose` (boolean, default false)

```bash
# Get hex
./soteria-cli getrawtransaction abc123...

# Get JSON details
./soteria-cli getrawtransaction abc123... 1
```

### `sendrawtransaction <hex>`

Broadcast a raw transaction to the network.  
**Parameters:** `hex` (string) – signed transaction hex

```bash
./soteria-cli sendrawtransaction "01000000..."
```

### `createrawtransaction [{"txid":..., "vout":...},...] {"address":amount,...}`

Create unsigned raw transaction from inputs and outputs.

```bash
./soteria-cli createrawtransaction \
  '[{"txid":"abc...","vout":0}]' \
  '{"Safr5V...":5}'
```

### `signrawtransaction <hex>`

Sign a raw transaction with wallet keys.  
**Parameters:** `hex` (string)

```bash
./soteria-cli signrawtransaction "01000000..."
```

---

## Network Commands

### `getconnectioncount`

Returns the number of connections to other nodes.

```bash
./soteria-cli getconnectioncount
```

### `getpeerinfo`

Returns data about each connected network node.

```bash
./soteria-cli getpeerinfo
```

### `addnode <node> <add|remove|onetry>`

Manually add or remove a peer connection.  
**Parameters:** `node` (string), `command` (string)

```bash
./soteria-cli addnode "89.105.213.189:8323" add
./soteria-cli addnode "178.72.89.199:8323" remove
```

---

## Mining Commands

### `getgenerate`

Returns the current mining status.

```bash
./soteria-cli getgenerate
```

### `setgenerate <generate> [genproclimit]`

Enables or disables mining.  
**Parameters:** `generate` (boolean), `genproclimit` (number, optional)

```bash
# Enable mining with 6 cores
./soteria-cli setgenerate true 6

# Disable mining
./soteria-cli setgenerate false
```

### `getmininginfo`

Returns mining statistics and network status.

```bash
./soteria-cli getmininginfo
```

**Example Response:**

```json
{
  "blocks": 666543,
  "currentblocksize": 0,
  "currentblocktx": 0,
  "difficulty": 0.50403396,
  "networkhashps": 32000,
  "chain": "main"
}
```

### `getblocktemplate`

Returns block template for mining. Used by mining pools and external miners.

```bash
./soteria-cli getblocktemplate
```

**Example Response:**

```json
{
  "version": 1,
  "previousblockhash": "8fb1e41d7112941ea7d0063fc94d0fd7b260b5fe6d73ec5d9eabdb484fff7dda",
  "transactions": [],
  "target": "0000000fffff...",
  "mutable": ["time", "transactions", "prevblock"],
  "sigoplimit": 240000,
  "sizelimit": 1000000,
  "height": 666543
}
```

### `submitblock <hexdata>`

Submit a mined block to the network.  
**Parameters:** `hexdata` (string) – hex-encoded block data

```bash
./soteria-cli submitblock "hex_block_data"
```

---

## Python Example

```python
import requests
import json

class SoteriaRPC:
    def __init__(self, user, password, host='127.0.0.1', port=8323):
        self.url = f'http://{host}:{port}/'
        self.auth = (user, password)
        self.headers = {'content-type': 'application/json'}
        self.id = 0

    def call(self, method, params=[]):
        self.id += 1
        payload = {
            'jsonrpc': '1.0',
            'id': self.id,
            'method': method,
            'params': params
        }
        response = requests.post(
            self.url,
            data=json.dumps(payload),
            headers=self.headers,
            auth=self.auth
        )
        return response.json()['result']

# Usage
rpc = SoteriaRPC('yourusername', 'yourpassword')
info = rpc.call('getinfo')
print(f"Balance: {info['balance']} SOTER")
print(f"Blocks: {info['blocks']}")
```

---

## Best Practices

- **Encrypt wallet** immediately after creation.
- **Backup `wallet.dat`** regularly and store multiple copies in secure locations.
- **Secure RPC:** Use strong passwords (32+ characters) and **never** expose RPC to the internet.
- **Run a full node:** Validate all consensus rules yourself – don't trust others.
- **Wait for confirmations:** 6 confirmations recommended for high-value transactions.
- **Keep software updated:** Follow releases for security patches.

> ⚠️ **Important:** If you lose `wallet.dat`, your coins are gone forever. There is **no recovery mechanism**. Make regular backups!

---

## Known Limitations

- **No P2P encryption** – network traffic is visible.
- **IRC peer discovery** is cleartext (use `-addnode` for better privacy).
- **Bloom filters** leak wallet addresses (SPV privacy trade-off).
```
