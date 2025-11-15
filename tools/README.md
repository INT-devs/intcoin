# INTcoin Development Tools

This directory contains utility scripts for INTcoin development and maintenance.

## Seed Node Manager

### Overview

The `update_seeds.py` script manages the hardcoded seed nodes in the INTcoin daemon. It provides an easy way to add, remove, and update seed nodes without manually editing the C++ source code.

### Files

- **`update_seeds.py`** - Python script for managing seed nodes
- **`seeds.json`** - JSON configuration file containing current seed nodes

### Usage

#### List Current Seed Nodes

```bash
python3 tools/update_seeds.py --list
```

Output:
```
Current seed nodes (3):
================================================================================
1. seed-us.international-coin.org:9333 (US node)
2. seed-uk.international-coin.org:9333 (UK node)
3. 2nrhdp7i4dricaf362hwnajj27lscbmimggvjetwjhuwgtdnfcurxzyd.onion:9333 (Tor node)
```

#### Add a New Seed Node

```bash
python3 tools/update_seeds.py --add "seed-eu.international-coin.org:9333" --comment "EU node"
```

This will:
1. Add the new seed to `src/daemon/main.cpp`
2. Update `tools/seeds.json` with the new seed
3. Preserve the comment for documentation

#### Remove a Seed Node

```bash
python3 tools/update_seeds.py --remove "seed-eu.international-coin.org:9333"
```

This will:
1. Remove the seed from `src/daemon/main.cpp`
2. Update `tools/seeds.json` to reflect the removal

#### Update from seeds.json

If you manually edit `seeds.json`, you can update the C++ source code with:

```bash
python3 tools/update_seeds.py --update
```

This will read all seeds from `seeds.json` and update `src/daemon/main.cpp` accordingly.

### seeds.json Format

The `seeds.json` file has the following structure:

```json
{
  "version": "1.0",
  "last_updated": "2025-11-15",
  "seeds": [
    {
      "address": "seed-us.international-coin.org",
      "port": 9333,
      "comment": "US node"
    },
    {
      "address": "seed-uk.international-coin.org",
      "port": 9333,
      "comment": "UK node"
    },
    {
      "address": "2nrhdp7i4dricaf362hwnajj27lscbmimggvjetwjhuwgtdnfcurxzyd.onion",
      "port": 9333,
      "comment": "Tor node"
    }
  ]
}
```

### Workflow for Adding New Seed Nodes

1. **Test the new seed node** - Ensure it's accessible and running INTcoin daemon
2. **Add using the script**:
   ```bash
   python3 tools/update_seeds.py --add "your-seed.international-coin.org:9333" --comment "Your location"
   ```
3. **Verify the changes**:
   ```bash
   python3 tools/update_seeds.py --list
   ```
4. **Rebuild the daemon**:
   ```bash
   cmake --build build --target intcoind -j4
   ```
5. **Test the updated daemon**:
   ```bash
   ./build/intcoind --help
   ```
6. **Commit the changes**:
   ```bash
   git add src/daemon/main.cpp tools/seeds.json
   git commit -m "Add new seed node: your-seed.international-coin.org"
   ```

### Current Seed Nodes (v1.1.0)

| Address | Port | Location | Type | Comment |
|---------|------|----------|------|---------|
| `seed-us.international-coin.org` | 9333 | United States | DNS | Primary US seed |
| `seed-uk.international-coin.org` | 9333 | United Kingdom | DNS | Primary UK seed |
| `2nrhdp7i4dricaf362hwnajj27lscbmimggvjetwjhuwgtdnfcurxzyd.onion` | 9333 | Tor Network | Onion | Tor bridge for privacy |

### Notes

- **DNS Seeds**: Regular domain-based seed nodes (e.g., `seed-us.international-coin.org`)
- **Tor Seeds**: Onion addresses for privacy-focused connections (e.g., `.onion` addresses)
- **Default Port**: 9333 (can be changed per seed)
- **Auto-Generated**: The seed nodes section in `main.cpp` is marked as auto-generated; manual edits may be overwritten

### Troubleshooting

#### Script Can't Find Repository Root

If you run the script from outside the repository:

```bash
python3 tools/update_seeds.py --repo-root /path/to/intcoin --list
```

#### Permission Denied

Make the script executable:

```bash
chmod +x tools/update_seeds.py
./tools/update_seeds.py --list
```

#### Invalid Address Format

Ensure addresses are in the format: `address:port` or just `address` (port defaults to 9333)

Valid examples:
- `seed-us.international-coin.org:9333`
- `seed-us.international-coin.org` (port 9333 assumed)
- `192.168.1.100:9333`
- `example.onion:9333`

## Future Tools

This directory will contain additional development tools as the project grows:

- **`benchmark.py`** - Performance benchmarking tools
- **`generate_docs.py`** - Documentation generation
- **`code_analysis.py`** - Code quality and security analysis
- **`release.py`** - Automated release management

## Contributing

When adding new tools to this directory:

1. Add a descriptive comment header to the script
2. Make the script executable (`chmod +x`)
3. Update this README with usage instructions
4. Add the tool to the CMake build if applicable

## License

All tools in this directory are released under the MIT License, same as INTcoin Core.
