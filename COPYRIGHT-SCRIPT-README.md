# Copyright Header Management Script

Automatically adds and updates copyright headers in INTcoin project files.

## Features

- ✅ Automatically adds copyright headers to new files
- ✅ Updates copyright year range (e.g., 2025 → 2025-2026)
- ✅ Extracts page titles from file content
- ✅ Supports multiple file types (Python, JavaScript, TypeScript, C++, CSS, HTML, Markdown)
- ✅ Dry-run mode to preview changes
- ✅ Recursive directory processing

## Copyright Header Format

The script adds headers in this format:

```
Copyright (c) 2025 INTcoin Team (Neil Adamson)
SPDX-License-Identifier: MIT License
[Page Title]
```

For C-style files (`.cpp`, `.js`, `.css`):
```cpp
/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Page Title
 */
```

For HTML/Markdown files:
```html
<!--
  Copyright (c) 2025 INTcoin Team (Neil Adamson)
  SPDX-License-Identifier: MIT License
  Page Title
-->
```

For Python files:
```python
# Copyright (c) 2025 INTcoin Team (Neil Adamson)
# SPDX-License-Identifier: MIT License
# Page Title
```

## Usage

### Basic Usage

Process current directory:
```bash
python3 add_copyright.py
```

Process specific directory:
```bash
python3 add_copyright.py /path/to/directory
```

Process single file:
```bash
python3 add_copyright.py website/index.html
```

### Options

**Dry Run** (preview changes without modifying files):
```bash
python3 add_copyright.py --dry-run
```

**Non-Recursive** (don't process subdirectories):
```bash
python3 add_copyright.py --no-recursive
```

**Custom Title** (for single file):
```bash
python3 add_copyright.py website/index.html --title "INTcoin Homepage"
```

### Examples

**Add copyright to all website files:**
```bash
python3 add_copyright.py website/
```

**Preview changes first:**
```bash
python3 add_copyright.py website/ --dry-run
```

**Update copyright years in all project files:**
```bash
python3 add_copyright.py .
```

**Process only branding files:**
```bash
python3 add_copyright.py branding/ --no-recursive
```

## Supported File Types

- **Python**: `.py`
- **JavaScript/TypeScript**: `.js`, `.ts`, `.jsx`, `.tsx`
- **C/C++**: `.c`, `.cpp`, `.h`, `.hpp`
- **Web**: `.html`, `.css`
- **Documentation**: `.md`

## Automatic Features

### Page Title Extraction

The script automatically extracts page titles:

- **HTML**: From `<title>` tag
- **Markdown**: From first `#` heading
- **Python**: From module docstring
- **Other**: From filename (converted to Title Case)

### Year Range Updates

When you run the script in future years:

- **2025**: `Copyright (c) 2025`
- **2026**: `Copyright (c) 2025-2026`
- **2027**: `Copyright (c) 2025-2027`

The script automatically updates existing headers to reflect the current year.

## Excluded Patterns

The script automatically skips:

- `.git/` directories
- `node_modules/` directories
- `venv/` directories
- `__pycache__/` directories
- `build/` directories
- `dist/` directories
- Minified files (`.min.js`, `.min.css`)

## Integration with Git

Add to your git workflow:

```bash
# Before committing new files
python3 add_copyright.py .

# Add to git
git add .
git commit -m "Add copyright headers"
```

You can also add this to a pre-commit hook:
```bash
#!/bin/sh
# .git/hooks/pre-commit
python3 add_copyright.py --dry-run
```

## License

This script is part of the INTcoin project and is licensed under the MIT License.

See [LICENSE](LICENSE) for full license text.
