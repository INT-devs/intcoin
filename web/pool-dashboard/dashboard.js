// INTcoin Mining Pool Dashboard JavaScript
// Version: 1.4.0

// Configuration
const CONFIG = {
    // RPC endpoint for pool statistics
    rpcUrl: 'http://localhost:2214/rpc',  // Pool RPC port
    // Update interval in milliseconds
    updateInterval: 30000,  // 30 seconds
    // API credentials (should be configured server-side)
    rpcUser: 'poolapi',
    rpcPassword: 'changeme'
};

// State
let currentWorkerAddress = null;
let updateTimer = null;
let isDarkTheme = false;

// Initialize dashboard on page load
document.addEventListener('DOMContentLoaded', () => {
    console.log('INTcoin Mining Pool Dashboard v1.4.0 initialized');

    // Load theme preference
    loadThemePreference();

    // Load initial data
    loadPoolStats();
    loadRecentBlocks();
    loadRecentPayments();
    loadTopMiners();

    // Start auto-refresh
    startAutoRefresh();

    // Load worker stats from localStorage if available
    const savedAddress = localStorage.getItem('workerAddress');
    if (savedAddress) {
        document.getElementById('worker-address').value = savedAddress;
        loadWorkerStats();
    }

    // Add enter key support for worker search
    document.getElementById('worker-address').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            loadWorkerStats();
        }
    });
});

// Theme toggle functionality
function toggleTheme() {
    isDarkTheme = !isDarkTheme;
    applyTheme();
    localStorage.setItem('darkTheme', isDarkTheme);
}

function loadThemePreference() {
    const saved = localStorage.getItem('darkTheme');
    if (saved !== null) {
        isDarkTheme = saved === 'true';
    } else {
        // Auto-detect system preference
        isDarkTheme = window.matchMedia('(prefers-color-scheme: dark)').matches;
    }
    applyTheme();
}

function applyTheme() {
    if (isDarkTheme) {
        document.documentElement.setAttribute('data-theme', 'dark');
        document.getElementById('theme-icon').innerHTML = '&#9788;'; // Sun icon
    } else {
        document.documentElement.removeAttribute('data-theme');
        document.getElementById('theme-icon').innerHTML = '&#9790;'; // Moon icon
    }
}

// Copy to clipboard functionality
function copyToClipboard(element) {
    const text = element.textContent;
    navigator.clipboard.writeText(text).then(() => {
        // Visual feedback
        element.classList.add('copied');
        const hint = element.nextElementSibling;
        const originalText = hint.textContent;
        hint.textContent = 'Copied!';

        setTimeout(() => {
            element.classList.remove('copied');
            hint.textContent = originalText;
        }, 2000);
    }).catch(err => {
        console.error('Failed to copy:', err);
    });
}

// Auto-refresh functionality
function startAutoRefresh() {
    updateTimer = setInterval(() => {
        loadPoolStats();
        loadRecentBlocks();
        loadRecentPayments();
        loadTopMiners();
        if (currentWorkerAddress) {
            loadWorkerStats();
        }
        updateLastRefreshTime();
    }, CONFIG.updateInterval);

    document.getElementById('refresh-status').textContent = 'Enabled (30s)';
}

function updateLastRefreshTime() {
    const now = new Date();
    document.getElementById('last-update').textContent = now.toLocaleTimeString();
}

// RPC call wrapper
async function rpcCall(method, params = []) {
    try {
        const response = await fetch(CONFIG.rpcUrl, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': 'Basic ' + btoa(CONFIG.rpcUser + ':' + CONFIG.rpcPassword)
            },
            body: JSON.stringify({
                jsonrpc: '2.0',
                id: Date.now(),
                method: method,
                params: params
            })
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        if (data.error) {
            throw new Error(data.error.message);
        }

        return data.result;
    } catch (error) {
        console.error(`RPC call failed for ${method}:`, error);
        // Return mock data for demonstration
        return getMockData(method);
    }
}

// Mock data for demonstration when RPC is unavailable
function getMockData(method) {
    const mockData = {
        'pool_getstats': {
            hashrate: 125000000,  // 125 MH/s
            difficulty: 1000000,
            miners: 42,
            blocks_found: 156,
            total_shares: 1234567,
            valid_shares_24h: 89234,
            block_height: 123456
        },
        'pool_getblocks': [
            {
                height: 123456,
                hash: 'a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6',
                timestamp: Date.now() - 3600000,
                finder: 'int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh',
                reward: 105113636,
                status: 'confirmed'
            },
            {
                height: 123455,
                hash: 'b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7',
                timestamp: Date.now() - 7200000,
                finder: 'int1q9876543210abcdefghijklmnopqrs',
                reward: 105113636,
                status: 'confirmed'
            },
            {
                height: 123454,
                hash: 'c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8',
                timestamp: Date.now() - 10800000,
                finder: 'int1qabcdef1234567890fedcba098765',
                reward: 105113636,
                status: 'pending'
            },
            {
                height: 123453,
                hash: 'd4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9',
                timestamp: Date.now() - 14400000,
                finder: 'int1qzxcvbnm0987654321asdfghjklqw',
                reward: 105113636,
                status: 'confirmed'
            }
        ],
        'pool_getpayments': [
            {
                timestamp: Date.now() - 1800000,
                address: 'int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh',
                amount: 5.25,
                txid: 'tx123456789abcdef0123456789abcdef'
            },
            {
                timestamp: Date.now() - 5400000,
                address: 'int1q9876543210abcdefghijklmnopqrs',
                amount: 10.50,
                txid: 'tx987654321fedcba9876543210fedcba'
            },
            {
                timestamp: Date.now() - 9000000,
                address: 'int1qabcdef1234567890fedcba098765',
                amount: 3.75,
                txid: 'txabcdef0123456789abcdef01234567'
            }
        ],
        'pool_gettopminers': [
            {
                rank: 1,
                address: 'int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh',
                hashrate: 45000000,
                shares: 12345
            },
            {
                rank: 2,
                address: 'int1q9876543210abcdefghijklmnopqrs',
                hashrate: 38000000,
                shares: 10234
            },
            {
                rank: 3,
                address: 'int1qabcdef1234567890fedcba098765',
                hashrate: 25000000,
                shares: 7890
            },
            {
                rank: 4,
                address: 'int1qzxcvbnm0987654321asdfghjklqw',
                hashrate: 17000000,
                shares: 5432
            },
            {
                rank: 5,
                address: 'int1qpoiuytrewq1234567890lkjhgfds',
                hashrate: 12000000,
                shares: 3456
            }
        ],
        'pool_getworker': {
            address: 'int1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh',
            hashrate: 12500000,
            shares: 5678,
            balance: 2.5,
            total_paid: 15.75,
            last_share: Date.now() - 60000,
            workers_online: 2
        }
    };

    return mockData[method] || null;
}

// Format hashrate
function formatHashrate(hashrate) {
    const units = ['H/s', 'KH/s', 'MH/s', 'GH/s', 'TH/s', 'PH/s'];
    let index = 0;
    let value = hashrate;

    while (value >= 1000 && index < units.length - 1) {
        value /= 1000;
        index++;
    }

    return value.toFixed(2) + ' ' + units[index];
}

// Format large numbers
function formatNumber(num) {
    return num.toLocaleString();
}

// Format timestamp
function formatTimestamp(timestamp) {
    const date = new Date(timestamp);
    return date.toLocaleString();
}

// Format relative time
function formatRelativeTime(timestamp) {
    const seconds = Math.floor((Date.now() - timestamp) / 1000);

    if (seconds < 60) return `${seconds}s ago`;
    if (seconds < 3600) return `${Math.floor(seconds / 60)}m ago`;
    if (seconds < 86400) return `${Math.floor(seconds / 3600)}h ago`;
    return `${Math.floor(seconds / 86400)}d ago`;
}

// Truncate address for display
function truncateAddress(address, start = 12, end = 8) {
    if (!address || address.length <= start + end) return address;
    return address.substring(0, start) + '...' + address.substring(address.length - end);
}

// Load pool statistics
async function loadPoolStats() {
    const stats = await rpcCall('pool_getstats');

    if (stats) {
        document.getElementById('pool-hashrate').textContent = formatHashrate(stats.hashrate);
        document.getElementById('network-diff').textContent = formatNumber(stats.difficulty);
        document.getElementById('active-miners').textContent = formatNumber(stats.miners);
        document.getElementById('blocks-found').textContent = formatNumber(stats.blocks_found);
        document.getElementById('total-shares').textContent = formatNumber(stats.total_shares);
        document.getElementById('valid-shares-24h').textContent = formatNumber(stats.valid_shares_24h);

        // Block height (new field)
        const blockHeightEl = document.getElementById('block-height');
        if (blockHeightEl && stats.block_height) {
            blockHeightEl.textContent = formatNumber(stats.block_height);
        }
    }

    updateLastRefreshTime();
}

// Load recent blocks
async function loadRecentBlocks() {
    const blocks = await rpcCall('pool_getblocks', [10]);
    const tbody = document.getElementById('blocks-table');

    if (!blocks || blocks.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" class="no-data">No blocks found yet</td></tr>';
        return;
    }

    tbody.innerHTML = blocks.map(block => `
        <tr>
            <td><strong>${formatNumber(block.height)}</strong></td>
            <td><code>${truncateAddress(block.hash, 16, 8)}</code></td>
            <td>${formatTimestamp(block.timestamp)}</td>
            <td><code>${truncateAddress(block.finder)}</code></td>
            <td>${(block.reward / 1000000000).toFixed(4)} INT</td>
            <td class="status-${block.status}">${block.status.toUpperCase()}</td>
        </tr>
    `).join('');
}

// Load recent payments
async function loadRecentPayments() {
    const payments = await rpcCall('pool_getpayments', [20]);
    const tbody = document.getElementById('payments-table');

    if (!payments || payments.length === 0) {
        tbody.innerHTML = '<tr><td colspan="4" class="no-data">No payments yet</td></tr>';
        return;
    }

    tbody.innerHTML = payments.map(payment => `
        <tr>
            <td>${formatTimestamp(payment.timestamp)}</td>
            <td><code>${truncateAddress(payment.address)}</code></td>
            <td><strong>${payment.amount.toFixed(4)} INT</strong></td>
            <td><code>${truncateAddress(payment.txid, 16, 8)}</code></td>
        </tr>
    `).join('');
}

// Load top miners
async function loadTopMiners() {
    const miners = await rpcCall('pool_gettopminers', [10]);
    const tbody = document.getElementById('top-miners-table');

    if (!miners || miners.length === 0) {
        tbody.innerHTML = '<tr><td colspan="4" class="no-data">No active miners</td></tr>';
        return;
    }

    tbody.innerHTML = miners.map((miner, index) => `
        <tr>
            <td><strong>${index + 1}</strong></td>
            <td><code>${truncateAddress(miner.address)}</code></td>
            <td>${formatHashrate(miner.hashrate)}</td>
            <td>${formatNumber(miner.shares)}</td>
        </tr>
    `).join('');
}

// Load worker statistics
async function loadWorkerStats() {
    const address = document.getElementById('worker-address').value.trim();

    if (!address) {
        alert('Please enter your INT address');
        return;
    }

    // Validate address format (basic check)
    if (!address.startsWith('int1')) {
        alert('Invalid address format. INTcoin addresses start with "int1"');
        return;
    }

    currentWorkerAddress = address;
    localStorage.setItem('workerAddress', address);

    const stats = await rpcCall('pool_getworker', [address]);

    if (!stats) {
        alert('Worker not found or RPC error');
        return;
    }

    // Show worker stats section
    document.getElementById('worker-stats').classList.remove('hidden');

    // Update worker stats
    document.getElementById('worker-hashrate').textContent = formatHashrate(stats.hashrate);
    document.getElementById('worker-shares').textContent = formatNumber(stats.shares);
    document.getElementById('worker-balance').textContent = stats.balance.toFixed(4) + ' INT';
    document.getElementById('worker-paid').textContent = stats.total_paid.toFixed(4) + ' INT';

    // Update worker details
    const lastShareEl = document.getElementById('worker-last-share');
    const workerCountEl = document.getElementById('worker-count');

    if (lastShareEl && stats.last_share) {
        lastShareEl.textContent = formatRelativeTime(stats.last_share);
    }
    if (workerCountEl && stats.workers_online !== undefined) {
        workerCountEl.textContent = stats.workers_online;
    }
}

// Export functions for debugging
window.poolDashboard = {
    loadPoolStats,
    loadRecentBlocks,
    loadRecentPayments,
    loadTopMiners,
    loadWorkerStats,
    rpcCall,
    toggleTheme,
    copyToClipboard
};
