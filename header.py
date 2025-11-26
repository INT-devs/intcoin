#!/usr/bin/env python3
"""
Copyright Header Management Script for INTcoin Project
Adds and updates copyright headers in source files
"""

import os
import sys
import re
from datetime import datetime
from pathlib import Path

# Configuration
COPYRIGHT_HOLDER = "INTcoin Team (Neil Adamson)"
LICENSE_NAME = "MIT License"
START_YEAR = 2025

# File type configurations
FILE_TYPES = {
    '.py': {'comment': '#', 'multiline': ('"""', '"""')},
    '.js': {'comment': '//', 'multiline': ('/*', ' */')},
    '.ts': {'comment': '//', 'multiline': ('/*', ' */')},
    '.jsx': {'comment': '//', 'multiline': ('/*', ' */')},
    '.tsx': {'comment': '//', 'multiline': ('/*', ' */')},
    '.cpp': {'comment': '//', 'multiline': ('/*', ' */')},
    '.hpp': {'comment': '//', 'multiline': ('/*', ' */')},
    '.h': {'comment': '//', 'multiline': ('/*', ' */')},
    '.c': {'comment': '//', 'multiline': ('/*', ' */')},
    '.css': {'comment': '//', 'multiline': ('/*', ' */')},
    '.html': {'comment': None, 'multiline': ('<!--', '-->')},
    '.md': {'comment': None, 'multiline': ('<!--', '-->')},
}


def get_current_year():
    """Get the current year"""
    return datetime.now().year


def get_copyright_year_range(start_year=START_YEAR):
    """Generate copyright year range string"""
    current_year = get_current_year()
    if current_year == start_year:
        return str(start_year)
    else:
        return f"{start_year}-{current_year}"


def extract_page_title(file_path, content):
    """Extract page title from file content"""
    ext = Path(file_path).suffix.lower()

    # For HTML files, try to extract <title> tag
    if ext == '.html':
        match = re.search(r'<title>(.*?)</title>', content, re.IGNORECASE)
        if match:
            return match.group(1).strip()

    # For Markdown files, try to extract first heading
    if ext == '.md':
        match = re.search(r'^#\s+(.+)$', content, re.MULTILINE)
        if match:
            return match.group(1).strip()

    # For Python files, try to extract module docstring
    if ext == '.py':
        match = re.search(r'^"""(.+?)"""', content, re.DOTALL | re.MULTILINE)
        if match:
            first_line = match.group(1).strip().split('\n')[0].strip()
            return first_line

    # Default: use filename without extension
    return Path(file_path).stem.replace('_', ' ').replace('-', ' ').title()


def generate_copyright_header(file_path, page_title=None, content=''):
    """Generate copyright header for a file"""
    ext = Path(file_path).suffix.lower()

    if ext not in FILE_TYPES:
        return None

    config = FILE_TYPES[ext]
    year_range = get_copyright_year_range()

    # Extract page title if not provided
    if not page_title:
        page_title = extract_page_title(file_path, content)

    # Build header lines
    header_lines = [
        f"Copyright (c) {year_range} {COPYRIGHT_HOLDER}",
        f"SPDX-License-Identifier: {LICENSE_NAME}",
        page_title
    ]

    # Format based on file type
    if config['multiline']:
        start_comment, end_comment = config['multiline']

        if ext == '.html' or ext == '.md':
            # HTML/Markdown style
            header = f"{start_comment}\n"
            for line in header_lines:
                header += f"  {line}\n"
            header += f"{end_comment}\n\n"
        else:
            # C-style multiline comment
            header = f"{start_comment}\n"
            for line in header_lines:
                header += f" * {line}\n"
            header += f" {end_comment}\n\n"

        return header

    elif config['comment']:
        # Single line comment style
        header = ""
        for line in header_lines:
            header += f"{config['comment']} {line}\n"
        header += "\n"
        return header

    return None


def has_copyright_header(content):
    """Check if content already has a copyright header"""
    # Look for copyright in first 20 lines
    lines = content.split('\n')[:20]
    for line in lines:
        if re.search(r'Copyright.*INTcoin.*Team', line, re.IGNORECASE):
            return True
    return False


def extract_existing_copyright_year(content):
    """Extract year range from existing copyright header"""
    match = re.search(r'Copyright.*?(\d{4})(?:-(\d{4}))?', content, re.IGNORECASE)
    if match:
        start = match.group(1)
        end = match.group(2)
        return (start, end if end else start)
    return None


def update_copyright_year(content):
    """Update copyright year in existing header"""
    current_year = get_current_year()

    # Pattern to match copyright line with year
    pattern = r'(Copyright.*?)(\d{4})(?:-\d{4})?(.*?INTcoin.*?Team)'

    def replace_year(match):
        prefix = match.group(1)
        start_year = match.group(2)
        suffix = match.group(3)

        if int(start_year) == current_year:
            return f"{prefix}{start_year}{suffix}"
        else:
            return f"{prefix}{start_year}-{current_year}{suffix}"

    updated_content = re.sub(pattern, replace_year, content, count=1)
    return updated_content


def add_or_update_copyright(file_path, dry_run=False):
    """Add or update copyright header in a file"""
    try:
        # Read file content
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Check if copyright exists
        if has_copyright_header(content):
            # Update year if needed
            updated_content = update_copyright_year(content)

            if updated_content != content:
                if dry_run:
                    print(f"  [DRY RUN] Would update copyright year in: {file_path}")
                else:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(updated_content)
                    print(f"  ✓ Updated copyright year in: {file_path}")
                return True
            else:
                print(f"  → Copyright already up-to-date: {file_path}")
                return False

        else:
            # Add new copyright header
            header = generate_copyright_header(file_path, content=content)

            if header:
                new_content = header + content

                if dry_run:
                    print(f"  [DRY RUN] Would add copyright header to: {file_path}")
                else:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(new_content)
                    print(f"  ✓ Added copyright header to: {file_path}")
                return True
            else:
                print(f"  ⚠ Unsupported file type: {file_path}")
                return False

    except Exception as e:
        print(f"  ✗ Error processing {file_path}: {e}")
        return False


def process_directory(directory, dry_run=False, recursive=True):
    """Process all files in a directory"""
    directory = Path(directory)

    # Patterns to exclude
    exclude_patterns = [
        '*.git*',
        '*node_modules*',
        '*venv*',
        '*__pycache__*',
        '*build*',
        '*dist*',
        '*.min.js',
        '*.min.css',
    ]

    # Supported extensions
    supported_exts = set(FILE_TYPES.keys())

    files_processed = 0
    files_updated = 0

    print(f"\nProcessing directory: {directory}")
    print(f"Mode: {'DRY RUN' if dry_run else 'LIVE'}")
    print("-" * 60)

    # Find all files
    pattern = '**/*' if recursive else '*'
    for file_path in directory.glob(pattern):
        if not file_path.is_file():
            continue

        # Skip excluded patterns
        if any(file_path.match(pattern) for pattern in exclude_patterns):
            continue

        # Skip unsupported extensions
        if file_path.suffix.lower() not in supported_exts:
            continue

        # Process file
        files_processed += 1
        if add_or_update_copyright(file_path, dry_run=dry_run):
            files_updated += 1

    print("-" * 60)
    print(f"Files processed: {files_processed}")
    print(f"Files updated: {files_updated}")
    print()


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(
        description='Add or update copyright headers in INTcoin project files'
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
        help='Show what would be done without making changes'
    )
    parser.add_argument(
        '--no-recursive',
        action='store_true',
        help='Do not process subdirectories'
    )
    parser.add_argument(
        '--title',
        help='Custom page title (only for single file)'
    )

    args = parser.parse_args()

    path = Path(args.path)

    if not path.exists():
        print(f"Error: Path does not exist: {path}")
        sys.exit(1)

    if path.is_file():
        # Process single file
        print(f"Processing file: {path}")
        print(f"Mode: {'DRY RUN' if args.dry_run else 'LIVE'}")
        print("-" * 60)

        add_or_update_copyright(path, dry_run=args.dry_run)

    else:
        # Process directory
        process_directory(
            path,
            dry_run=args.dry_run,
            recursive=not args.no_recursive
        )


if __name__ == '__main__':
    main()
