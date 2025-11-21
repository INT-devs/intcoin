#!/usr/bin/env python3
# Copyright (c) 2025 INTcoin Core (Maddison Lane)
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# INTcoin Seed Node Manager

"""
INTcoin Seed Node Manager

This script manages the hardcoded seed nodes in the INTcoin daemon.
It updates src/daemon/main.cpp with the current list of seed nodes.

Usage:
    # List current seed nodes
    python3 tools/update_seeds.py --list

    # Add a new seed node
    python3 tools/update_seeds.py --add "seed-eu.international-coin.org:9333" --comment "EU node"

    # Remove a seed node
    python3 tools/update_seeds.py --remove "seed-eu.international-coin.org:9333"

    # Update from seeds.json file
    python3 tools/update_seeds.py --update

    # Show help
    python3 tools/update_seeds.py --help
"""

import sys
import re
import argparse
import json
from pathlib import Path
from typing import List, Tuple


class SeedNode:
    """Represents a seed node with address, port, and description"""

    def __init__(self, address: str, port: int = 9333, comment: str = ""):
        self.address = address
        self.port = port
        self.comment = comment

    def to_cpp_string(self) -> str:
        """Convert to C++ string format"""
        if self.comment:
            return f'"{self.address}:{self.port}",  // {self.comment}'
        return f'"{self.address}:{self.port}"'

    def __str__(self) -> str:
        return f"{self.address}:{self.port} ({self.comment})" if self.comment else f"{self.address}:{self.port}"

    @staticmethod
    def from_cpp_string(cpp_line: str) -> 'SeedNode':
        """Parse a C++ seed node line"""
        # Match pattern: "address:port",  // comment
        match = re.match(r'\s*"([^:]+):(\d+)"(?:,\s*//\s*(.+))?', cpp_line)
        if match:
            address = match.group(1)
            port = int(match.group(2))
            comment = match.group(3).strip() if match.group(3) else ""
            return SeedNode(address, port, comment)
        return None

    @staticmethod
    def from_dict(data: dict) -> 'SeedNode':
        """Create from dictionary"""
        return SeedNode(
            address=data['address'],
            port=data.get('port', 9333),
            comment=data.get('comment', '')
        )

    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            'address': self.address,
            'port': self.port,
            'comment': self.comment
        }


class SeedNodeManager:
    """Manages seed nodes in the INTcoin daemon source code"""

    def __init__(self, repo_root: Path = None):
        if repo_root is None:
            # Find repo root by looking for CMakeLists.txt
            # Start from script directory and go up
            current = Path(__file__).resolve().parent.parent  # Go up from tools/ to repo root
            if (current / 'CMakeLists.txt').exists():
                repo_root = current
            else:
                # Fallback: search upwards
                current = Path(__file__).resolve().parent
                while current != current.parent:
                    if (current / 'CMakeLists.txt').exists():
                        repo_root = current
                        break
                    current = current.parent
            if repo_root is None:
                raise ValueError("Could not find repository root")

        self.repo_root = Path(repo_root)
        self.main_cpp = self.repo_root / 'src' / 'daemon' / 'main.cpp'
        self.seeds_json = self.repo_root / 'tools' / 'seeds.json'

        if not self.main_cpp.exists():
            raise FileNotFoundError(f"Could not find {self.main_cpp}")

    def read_seeds_from_cpp(self) -> List[SeedNode]:
        """Read current seed nodes from main.cpp"""
        with open(self.main_cpp, 'r') as f:
            content = f.read()

        # Find the seed nodes section
        pattern = r'const std::vector<std::string> seed_nodes = \{(.*?)\};'
        match = re.search(pattern, content, re.DOTALL)

        if not match:
            print("Warning: Could not find seed_nodes vector in main.cpp", file=sys.stderr)
            return []

        seeds_section = match.group(1)
        seeds = []

        for line in seeds_section.split('\n'):
            line = line.strip()
            if line and not line.startswith('//'):
                seed = SeedNode.from_cpp_string(line)
                if seed:
                    seeds.append(seed)

        return seeds

    def write_seeds_to_cpp(self, seeds: List[SeedNode]) -> bool:
        """Update main.cpp with new seed nodes"""
        with open(self.main_cpp, 'r') as f:
            content = f.read()

        # Generate new seed nodes section
        seed_lines = []
        for i, seed in enumerate(seeds):
            # Add comma except for last item
            line = seed.to_cpp_string()
            if i < len(seeds) - 1:
                if not line.endswith(','):
                    # Add comma if not already present
                    line = line.rstrip() if line.endswith(',') else line + ','
            else:
                # Remove trailing comma from last item
                line = line.rstrip(',').rstrip()
            seed_lines.append(f'            {line}')

        new_seeds_section = '\n'.join(seed_lines)

        # Replace the seed nodes section
        new_content = f"""        // Add hardcoded seed nodes
        // This section is auto-generated by tools/update_seeds.py
        const std::vector<std::string> seed_nodes = {{
{new_seeds_section}
        }};"""

        pattern = r'// Add hardcoded seed nodes.*?const std::vector<std::string> seed_nodes = \{.*?\};'

        updated_content = re.sub(pattern, new_content, content, flags=re.DOTALL)

        if updated_content == content:
            print("Warning: No changes were made to main.cpp", file=sys.stderr)
            return False

        # Write back to file
        with open(self.main_cpp, 'w') as f:
            f.write(updated_content)

        return True

    def read_seeds_from_json(self) -> List[SeedNode]:
        """Read seed nodes from seeds.json"""
        if not self.seeds_json.exists():
            return []

        with open(self.seeds_json, 'r') as f:
            data = json.load(f)

        return [SeedNode.from_dict(node) for node in data.get('seeds', [])]

    def write_seeds_to_json(self, seeds: List[SeedNode]):
        """Write seed nodes to seeds.json"""
        data = {
            'version': '1.0',
            'last_updated': '2025-11-15',
            'seeds': [seed.to_dict() for seed in seeds]
        }

        with open(self.seeds_json, 'w') as f:
            json.dump(data, f, indent=2)

    def list_seeds(self):
        """List all current seed nodes"""
        seeds = self.read_seeds_from_cpp()

        if not seeds:
            print("No seed nodes found.")
            return

        print(f"Current seed nodes ({len(seeds)}):")
        print("=" * 80)
        for i, seed in enumerate(seeds, 1):
            print(f"{i}. {seed}")
        print()

    def add_seed(self, address: str, port: int = 9333, comment: str = ""):
        """Add a new seed node"""
        seeds = self.read_seeds_from_cpp()

        # Check if already exists
        for seed in seeds:
            if seed.address == address and seed.port == port:
                print(f"Error: Seed node {address}:{port} already exists", file=sys.stderr)
                return False

        # Add new seed
        new_seed = SeedNode(address, port, comment)
        seeds.append(new_seed)

        # Update files
        if self.write_seeds_to_cpp(seeds):
            self.write_seeds_to_json(seeds)
            print(f"✓ Added seed node: {new_seed}")
            return True

        return False

    def remove_seed(self, address: str, port: int = 9333):
        """Remove a seed node"""
        seeds = self.read_seeds_from_cpp()

        # Find and remove
        original_count = len(seeds)
        seeds = [s for s in seeds if not (s.address == address and s.port == port)]

        if len(seeds) == original_count:
            print(f"Error: Seed node {address}:{port} not found", file=sys.stderr)
            return False

        # Update files
        if self.write_seeds_to_cpp(seeds):
            self.write_seeds_to_json(seeds)
            print(f"✓ Removed seed node: {address}:{port}")
            return True

        return False

    def update_from_json(self):
        """Update main.cpp from seeds.json"""
        seeds = self.read_seeds_from_json()

        if not seeds:
            print("Error: No seeds found in seeds.json", file=sys.stderr)
            return False

        if self.write_seeds_to_cpp(seeds):
            print(f"✓ Updated main.cpp with {len(seeds)} seed nodes from seeds.json")
            return True

        return False


def main():
    parser = argparse.ArgumentParser(
        description='Manage INTcoin seed nodes',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # List current seed nodes
  python3 tools/update_seeds.py --list

  # Add a new seed node
  python3 tools/update_seeds.py --add "seed-eu.international-coin.org:9333" --comment "EU node"

  # Remove a seed node
  python3 tools/update_seeds.py --remove "seed-eu.international-coin.org:9333"

  # Update from seeds.json
  python3 tools/update_seeds.py --update
        """
    )

    parser.add_argument('--list', action='store_true', help='List current seed nodes')
    parser.add_argument('--add', metavar='ADDRESS[:PORT]', help='Add a new seed node')
    parser.add_argument('--remove', metavar='ADDRESS[:PORT]', help='Remove a seed node')
    parser.add_argument('--comment', help='Comment for new seed node (used with --add)')
    parser.add_argument('--update', action='store_true', help='Update from seeds.json')
    parser.add_argument('--repo-root', help='Repository root directory (auto-detected by default)')

    args = parser.parse_args()

    try:
        manager = SeedNodeManager(args.repo_root)

        if args.list:
            manager.list_seeds()
        elif args.add:
            # Parse address and port
            parts = args.add.split(':')
            address = parts[0]
            port = int(parts[1]) if len(parts) > 1 else 9333
            comment = args.comment or ""

            manager.add_seed(address, port, comment)
        elif args.remove:
            # Parse address and port
            parts = args.remove.split(':')
            address = parts[0]
            port = int(parts[1]) if len(parts) > 1 else 9333

            manager.remove_seed(address, port)
        elif args.update:
            manager.update_from_json()
        else:
            parser.print_help()
            return 1

        return 0

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
