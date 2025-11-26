# Copyright Script Testing Guide

## How to Test Year Updates

To test that the copyright year updates correctly:

### 1. View Current Copyright Headers

```bash
# Check a file with copyright header
head -5 website/index.html
```

You should see:
```html
<!--
  Copyright (c) 2025 INTcoin Team (Neil Adamson)
  SPDX-License-Identifier: MIT License
  INTcoin - Quantum-Resistant • Decentralized
-->
```

### 2. Simulate Future Year (for testing)

To test the year update feature, you can temporarily modify the script:

**Option A: Modify the script temporarily**
```python
# In add_copyright.py, change line:
def get_current_year():
    return datetime.now().year

# To:
def get_current_year():
    return 2026  # or 2027, 2028, etc.
```

**Option B: Test when the year actually changes**

Simply wait until 2026 and run:
```bash
python3 add_copyright.py website/
```

The output will show:
```
Processing directory: website
Mode: LIVE
------------------------------------------------------------
  ✓ Updated copyright year in: website/index.html
  ✓ Updated copyright year in: website/README.md
  ✓ Updated copyright year in: website/css/styles.css
------------------------------------------------------------
Files processed: 3
Files updated: 3
```

And the copyright will be updated to:
```
Copyright (c) 2025-2026 INTcoin Team (Neil Adamson)
```

### 3. Verify Updates

```bash
# Check that year was updated
head -5 website/index.html
```

### 4. Running in Production

**Recommended workflow:**

1. **Before committing new files:**
   ```bash
   python3 add_copyright.py .
   ```

2. **Annual update (run at start of new year):**
   ```bash
   # Preview changes
   python3 add_copyright.py . --dry-run

   # Apply updates
   python3 add_copyright.py .

   # Commit changes
   git add .
   git commit -m "Update copyright year to $(date +%Y)"
   ```

3. **Continuous Integration:**
   Add to your CI/CD pipeline to check copyright headers:
   ```yaml
   # .github/workflows/copyright.yml
   name: Check Copyright Headers
   on: [push, pull_request]
   jobs:
     check:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v2
         - name: Check copyright headers
           run: python3 add_copyright.py . --dry-run
   ```

## Expected Behavior

### First Run (2025)
- **Before:** No copyright header
- **After:** `Copyright (c) 2025 INTcoin Team (Neil Adamson)`

### Second Run (Still 2025)
- **Before:** `Copyright (c) 2025 INTcoin Team (Neil Adamson)`
- **After:** No change (already up-to-date)

### Third Run (Now 2026)
- **Before:** `Copyright (c) 2025 INTcoin Team (Neil Adamson)`
- **After:** `Copyright (c) 2025-2026 INTcoin Team (Neil Adamson)`

### Fourth Run (Now 2027)
- **Before:** `Copyright (c) 2025-2026 INTcoin Team (Neil Adamson)`
- **After:** `Copyright (c) 2025-2027 INTcoin Team (Neil Adamson)`

## Verification Commands

```bash
# Count files with copyright headers
grep -r "Copyright.*INTcoin Team" website/ | wc -l

# List all files with copyright headers
grep -r "Copyright.*INTcoin Team" website/ -l

# Show copyright lines from all files
grep -r "Copyright.*INTcoin Team" website/

# Check if any headers need updating (when run in 2026+)
python3 add_copyright.py . --dry-run | grep "Would update"
```

## Troubleshooting

### Script doesn't find files

Make sure you're in the correct directory:
```bash
cd /Users/neiladamson/Desktop/intcoin
python3 add_copyright.py website/
```

### Script skips certain files

The script only processes supported file types. Check [COPYRIGHT-SCRIPT-README.md](COPYRIGHT-SCRIPT-README.md) for the list of supported extensions.

### Copyright not updating

1. Check that the copyright line matches the expected format:
   ```
   Copyright (c) YYYY INTcoin Team (Neil Adamson)
   ```

2. Run with verbose output:
   ```bash
   python3 add_copyright.py website/ --dry-run
   ```

3. Check file permissions:
   ```bash
   ls -la website/
   chmod 644 website/*.html
   ```
