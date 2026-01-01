// Bridge Transaction List Component

import { useBridgeTransactions } from '@/hooks/useBridge'
import type { BridgeTransaction } from '@/types/bridge'

interface TransactionListProps {
  address?: string
  type?: 'deposit' | 'withdrawal' | 'all'
  limit?: number
}

export function TransactionList({
  address,
  type = 'all',
  limit = 50,
}: TransactionListProps) {
  const { data, isLoading, error } = useBridgeTransactions(address, type, limit, 0)

  if (isLoading) {
    return <div className="tx-loading">Loading transactions...</div>
  }

  if (error) {
    return (
      <div className="tx-error">
        Failed to load transactions: {error instanceof Error ? error.message : 'Unknown error'}
      </div>
    )
  }

  if (!data || data.transactions.length === 0) {
    return <div className="tx-empty">No transactions found</div>
  }

  return (
    <div className="transaction-list">
      <h3>Recent Transactions ({data.total})</h3>
      <div className="transactions-table">
        <table>
          <thead>
            <tr>
              <th>Type</th>
              <th>Token</th>
              <th>Amount</th>
              <th>Chain</th>
              <th>Status</th>
              <th>Date</th>
              <th>TX ID</th>
            </tr>
          </thead>
          <tbody>
            {data.transactions.map((tx) => (
              <TransactionRow key={tx.tx_id} transaction={tx} />
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}

function TransactionRow({ transaction }: { transaction: BridgeTransaction }) {
  const chain = transaction.source_chain || transaction.destination_chain || '—'
  const date = new Date(transaction.timestamp * 1000).toLocaleString()

  return (
    <tr className={`tx-row tx-${transaction.type}`}>
      <td className="tx-type">
        <span className={`badge badge-${transaction.type}`}>
          {transaction.type.toUpperCase()}
        </span>
      </td>
      <td className="tx-token">{transaction.token}</td>
      <td className="tx-amount">{formatAmount(transaction.amount)}</td>
      <td className="tx-chain">{chain}</td>
      <td className="tx-status">
        <span className={`status-badge status-${transaction.status}`}>
          {transaction.status}
        </span>
        {transaction.confirmations !== undefined && (
          <span className="confirmations">
            {transaction.confirmations} conf
          </span>
        )}
        {transaction.signatures !== undefined && (
          <span className="signatures">
            {transaction.signatures}/{transaction.required_signatures} sigs
          </span>
        )}
      </td>
      <td className="tx-date">{date}</td>
      <td className="tx-id">
        <code title={transaction.tx_id}>
          {transaction.tx_id.slice(0, 8)}...{transaction.tx_id.slice(-8)}
        </code>
      </td>
    </tr>
  )
}

function formatAmount(amount: string): string {
  const num = BigInt(amount)
  const btc = Number(num) / 1e8
  return btc.toFixed(8)
}
