#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Automatic header comment management script.

import os
import sys
import re
from datetime import datetime
from pathlib import Path
from typing import List, Set

# Directories to ignore (dependencies and build artifacts)
IGNORE_DIRS: Set[str] = {
    'build', 'build-*', 'cmake-build-*', 'out', 'bin', 'lib',
    'external', 'contrib', 'node_modules', '__pycache__',
    '.git', '.vscode', '.idea', 'venv', 'env', 'ENV',
    'datadir', 'testnet', 'mainnet', 'regtest'
}

# File extensions to process
EXTENSIONS: Set[str] = {'.cpp', '.h', '.hpp', '.py', '.c', '.cc'}

# Extensions with different comment styles
CPP_EXTENSIONS: Set[str] = {'.cpp', '.h', '.hpp', '.c', '.cc'}
PYTHON_EXTENSIONS: Set[str] = {'.py'}

CURRENT_YEAR = datetime.now().year


def should_ignore_path(path: Path) -> bool:
    """Check if path should be ignored based on IGNORE_DIRS."""
    parts = path.parts
    for part in parts:
        for ignore_pattern in IGNORE_DIRS:
            if ignore_pattern.endswith('*'):
                if part.startswith(ignore_pattern[:-1]):
                    return True
            elif part == ignore_pattern:
                return True
    return False


def extract_year_from_header(content: str, is_cpp: bool) -> tuple:
    """Extract existing year information from header if present."""
    if is_cpp:
        pattern = r'//\s*Copyright\s*\(c\)\s*(\d{4})(?:\s*-\s*(\d{4}))?\s*INTcoin\s*Core'
    else:
        pattern = r'#\s*Copyright\s*\(c\)\s*(\d{4})(?:\s*-\s*(\d{4}))?\s*INTcoin\s*Core'

    match = re.search(pattern, content)
    if match:
        start_year = int(match.group(1))
        end_year = int(match.group(2)) if match.group(2) else start_year
        return (start_year, end_year)
    return None


def generate_cpp_header(start_year: int, page_title: str = "") -> str:
    """Generate C++ style header comment."""
    if start_year < CURRENT_YEAR:
        copyright_line = f"// Copyright (c) {start_year} - {CURRENT_YEAR} INTcoin Core (Maddison Lane)"
    else:
        copyright_line = f"// Copyright (c) {CURRENT_YEAR} INTcoin Core (Maddison Lane)"

    header = f"""{copyright_line}
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""

    if page_title:
        header += f"//\n// {page_title}\n"

    return header + "\n"


def generate_python_header(start_year: int, page_title: str = "", has_shebang: bool = False) -> str:
    """Generate Python style header comment."""
    if start_year < CURRENT_YEAR:
        copyright_line = f"# Copyright (c) {start_year} - {CURRENT_YEAR} INTcoin Core (Maddison Lane)"
    else:
        copyright_line = f"# Copyright (c) {CURRENT_YEAR} INTcoin Core (Maddison Lane)"

    header = f"""{copyright_line}
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""

    if page_title:
        header += f"#\n# {page_title}\n"

    return header + "\n"


def remove_existing_header(content: str, is_cpp: bool) -> tuple:
    """Remove existing INTcoin header and return cleaned content with shebang info."""
    lines = content.split('\n')
    has_shebang = False
    shebang = ""

    # Check for shebang
    if lines and lines[0].startswith('#!'):
        has_shebang = True
        shebang = lines[0] + '\n'
        lines = lines[1:]

    # Remove existing header
    if is_cpp:
        pattern = r'^//\s*Copyright\s*\(c\)'
    else:
        pattern = r'^#\s*Copyright\s*\(c\)'

    # Skip header lines
    i = 0
    while i < len(lines):
        if re.match(pattern, lines[i].strip()):
            # Found start of header, skip until we find non-comment or empty line after header
            while i < len(lines) and (lines[i].strip().startswith('//' if is_cpp else '#') or not lines[i].strip()):
                i += 1
            break
        elif not lines[i].strip():
            i += 1
        else:
            break

    cleaned_content = '\n'.join(lines[i:])
    return (cleaned_content, has_shebang, shebang)


def process_file(file_path: Path, page_title: str = "", dry_run: bool = False) -> bool:
    """Process a single file to add/update header."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return False

    is_cpp = file_path.suffix in CPP_EXTENSIONS

    # Check for existing year
    existing_year_info = extract_year_from_header(content, is_cpp)
    start_year = existing_year_info[0] if existing_year_info else CURRENT_YEAR

    # Remove existing header
    cleaned_content, has_shebang, shebang = remove_existing_header(content, is_cpp)

    # Generate new header
    if is_cpp:
        new_header = generate_cpp_header(start_year, page_title)
        new_content = new_header + cleaned_content
    else:
        new_header = generate_python_header(start_year, page_title, has_shebang)
        new_content = (shebang if has_shebang else "") + new_header + cleaned_content

    if dry_run:
        print(f"Would update: {file_path}")
        return True

    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated: {file_path}")
        return True
    except Exception as e:
        print(f"Error writing {file_path}: {e}")
        return False


def process_directory(root_dir: Path, dry_run: bool = False) -> tuple:
    """Process all files in directory recursively."""
    processed = 0
    skipped = 0

    for file_path in root_dir.rglob('*'):
        if not file_path.is_file():
            continue

        if file_path.suffix not in EXTENSIONS:
            continue

        if should_ignore_path(file_path):
            skipped += 1
            continue

        if process_file(file_path, dry_run=dry_run):
            processed += 1

    return (processed, skipped)


def main():
    """Main entry point."""
    import argparse

    parser = argparse.ArgumentParser(
        description='Add/update INTcoin copyright headers to source files'
    )
    parser.add_argument(
        'path',
        nargs='?',
        default='.',
        help='File or directory to process (default: current directory)'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be changed without making changes'
    )
    parser.add_argument(
        '--title',
        default='',
        help='Page title to add to header'
    )

    args = parser.parse_args()

    path = Path(args.path).resolve()

    if not path.exists():
        print(f"Error: Path does not exist: {path}")
        sys.exit(1)

    if path.is_file():
        if path.suffix not in EXTENSIONS:
            print(f"Error: File type not supported: {path.suffix}")
            sys.exit(1)

        if process_file(path, args.title, args.dry_run):
            print("Done!")
        else:
            sys.exit(1)
    else:
        print(f"Processing directory: {path}")
        processed, skipped = process_directory(path, args.dry_run)
        print(f"\nProcessed: {processed} files")
        print(f"Skipped: {skipped} files")
        print("Done!")


if __name__ == '__main__':
    main()
