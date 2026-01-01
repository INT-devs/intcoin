// INTcoin Bridge Web UI - Main App Component

import { useState } from 'react'
import { QueryClient, QueryClientProvider } from '@tanstack/react-query'
import { BridgeStats } from '@/components/BridgeStats'
import { DepositForm } from '@/components/DepositForm'
import { WithdrawalForm } from '@/components/WithdrawalForm'
import { TransactionList } from '@/components/TransactionList'
import './App.css'

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      refetchOnWindowFocus: false,
      retry: 1,
    },
  },
})

type Tab = 'deposit' | 'withdraw' | 'transactions'

function BridgeApp() {
  const [activeTab, setActiveTab] = useState<Tab>('deposit')
  const [userAddress, setUserAddress] = useState('')

  return (
    <div className="bridge-app">
      <header className="app-header">
        <div className="header-content">
          <h1>🌉 INTcoin Cross-Chain Bridge</h1>
          <p className="subtitle">
            Bridge Bitcoin, Ethereum, and Litecoin to INTcoin
          </p>
        </div>
      </header>

      <div className="app-container">
        <section className="stats-section">
          <BridgeStats address={userAddress} />
        </section>

        <div className="address-input-section">
          <label htmlFor="userAddress">Your INTcoin Address (optional)</label>
          <input
            id="userAddress"
            type="text"
            placeholder="int1..."
            value={userAddress}
            onChange={(e) => setUserAddress(e.target.value)}
            className="address-input"
          />
        </div>

        <section className="main-section">
          <div className="tabs">
            <button
              className={`tab ${activeTab === 'deposit' ? 'active' : ''}`}
              onClick={() => setActiveTab('deposit')}
            >
              Deposit
            </button>
            <button
              className={`tab ${activeTab === 'withdraw' ? 'active' : ''}`}
              onClick={() => setActiveTab('withdraw')}
            >
              Withdraw
            </button>
            <button
              className={`tab ${activeTab === 'transactions' ? 'active' : ''}`}
              onClick={() => setActiveTab('transactions')}
            >
              Transactions
            </button>
          </div>

          <div className="tab-content">
            {activeTab === 'deposit' && <DepositForm />}
            {activeTab === 'withdraw' && <WithdrawalForm />}
            {activeTab === 'transactions' && (
              <TransactionList address={userAddress} />
            )}
          </div>
        </section>

        <footer className="app-footer">
          <p>
            INTcoin Bridge v1.2.0-beta | Pure Proof-of-Work | Post-Quantum Secure
          </p>
          <p className="warning">
            ⚠️ This is beta software. Use at your own risk. Always verify transactions.
          </p>
        </footer>
      </div>
    </div>
  )
}

export default function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <BridgeApp />
    </QueryClientProvider>
  )
}
