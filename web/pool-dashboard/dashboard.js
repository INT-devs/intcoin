// INTcoin Mining Pool Dashboard JavaScript

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

// Initialize dashboard on page load
document.addEventListener('DOMContentLoaded', () => {
    console.log('INTcoin Mining Pool Dashboard initialized');
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
});

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
            valid_shares_24h: 89234
        },
        'pool_getblocks': [
            {
                height: 123456,
                hash: 'a1b2c3d4...e5f6g7h8',
                timestamp: Date.now() - 3600000,
                finder: 'intc1qxy2kgdygjrs...',
                reward: 105113636,
                status: 'confirmed'
            },
            {
                height: 123455,
                hash: 'b2c3d4e5...f6g7h8i9',
                timestamp: Date.now() - 7200000,
                finder: 'intc1q9876543210...',
                reward: 105113636,
                status: 'confirmed'
            },
            {
                height: 123454,
                hash: 'c3d4e5f6...g7h8i9j0',
                timestamp: Date.now() - 10800000,
                finder: 'intc1qabcdef12345...',
                reward: 105113636,
                status: 'pending'
            }
        ],
        'pool_getpayments': [
            {
                timestamp: Date.now() - 1800000,
                address: 'intc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh',
                amount: 5.25,
                txid: 'tx123456789abcdef'
            },
            {
                timestamp: Date.now() - 5400000,
                address: 'intc1q9876543210abcdefghijklmnopqrstuvwxyz',
                amount: 10.50,
                txid: 'tx987654321fedcba'
            }
        ],
        'pool_gettopminers': [
            {
                rank: 1,
                address: 'intc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh',
                hashrate: 45000000,
                shares: 12345
            },
            {
                rank: 2,
                address: 'intc1q9876543210abcdefghijklmnopqrstuvwxyz',
                hashrate: 38000000,
                shares: 10234
            },
            {
                rank: 3,
                address: 'intc1qabcdef1234567890fedcba0987654321xyz',
                hashrate: 25000000,
                shares: 7890
            }
        ],
        'pool_getworker': {
            address: 'intc1qxy2kgdygjrsqtzq2n0yrf2493p83kkfjhx0wlh',
            hashrate: 12500000,
            shares: 5678,
            balance: 2.5,
            total_paid: 15.75
        }
    };

    return mockData[method] || null;
}

// Format hashrate
function formatHashrate(hashrate) {
    const units = ['H/s', 'KH/s', 'MH/s', 'GH/s', 'TH/s'];
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
            <td>${formatNumber(block.height)}</td>
            <td><code>${truncateAddress(block.hash, 16, 8)}</code></td>
            <td>${formatTimestamp(block.timestamp)}</td>
            <td><code>${truncateAddress(block.finder)}</code></td>
            <td>${(block.reward / 1000000000).toFixed(2)} INT</td>
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
            <td>${payment.amount.toFixed(4)} INT</td>
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
            <td>${index + 1}</td>
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
}

// Export functions for debugging
window.poolDashboard = {
    loadPoolStats,
    loadRecentBlocks,
    loadRecentPayments,
    loadTopMiners,
    loadWorkerStats,
    rpcCall
};
