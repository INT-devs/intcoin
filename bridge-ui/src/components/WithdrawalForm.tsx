// Bridge Withdrawal Form Component

import { useState } from 'react'
import { useWithdrawal } from '@/hooks/useBridge'
import type { BridgeChain, WrappedToken } from '@/types/bridge'

export function WithdrawalForm() {
  const [chain, setChain] = useState<BridgeChain>('bitcoin')
  const [token, setToken] = useState<WrappedToken>('wBTC')
  const [destination, setDestination] = useState('')
  const [amount, setAmount] = useState('')
  const [signature, setSignature] = useState('')

  const withdrawal = useWithdrawal()

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()

    try {
      const result = await withdrawal.mutateAsync({
        chain,
        token,
        destination,
        amount,
        signature,
      })

      alert(
        `Withdrawal requested successfully!\n` +
        `Withdrawal ID: ${result.withdrawal_id}\n` +
        `Status: ${result.status}\n` +
        `Signatures: ${result.current_signatures}/${result.required_signatures}`
      )

      // Reset form
      setDestination('')
      setAmount('')
      setSignature('')
    } catch (error) {
      alert(`Withdrawal failed: ${error instanceof Error ? error.message : 'Unknown error'}`)
    }
  }

  // Update token when chain changes
  const handleChainChange = (newChain: BridgeChain) => {
    setChain(newChain)
    if (newChain === 'bitcoin') setToken('wBTC')
    else if (newChain === 'ethereum') setToken('wETH')
    else if (newChain === 'litecoin') setToken('wLTC')
  }

  return (
    <div className="withdrawal-form">
      <h3>Withdraw from Bridge</h3>
      <form onSubmit={handleSubmit}>
        <div className="form-group">
          <label htmlFor="chain">Destination Chain</label>
          <select
            id="chain"
            value={chain}
            onChange={(e) => handleChainChange(e.target.value as BridgeChain)}
            required
          >
            <option value="bitcoin">Bitcoin</option>
            <option value="ethereum">Ethereum</option>
            <option value="litecoin">Litecoin</option>
          </select>
        </div>

        <div className="form-group">
          <label htmlFor="token">Wrapped Token</label>
          <input
            id="token"
            type="text"
            value={token}
            readOnly
            className="readonly"
          />
        </div>

        <div className="form-group">
          <label htmlFor="destination">Destination Address</label>
          <input
            id="destination"
            type="text"
            placeholder="0x... or bc1..."
            value={destination}
            onChange={(e) => setDestination(e.target.value)}
            required
          />
        </div>

        <div className="form-group">
          <label htmlFor="amount">Amount (satoshis/wei)</label>
          <input
            id="amount"
            type="text"
            placeholder="100000000"
            value={amount}
            onChange={(e) => setAmount(e.target.value)}
            required
          />
        </div>

        <div className="form-group">
          <label htmlFor="signature">Signature (hex)</label>
          <textarea
            id="signature"
            placeholder="0x..."
            value={signature}
            onChange={(e) => setSignature(e.target.value)}
            rows={3}
            required
          />
        </div>

        <button
          type="submit"
          disabled={withdrawal.isPending}
          className="btn-primary"
        >
          {withdrawal.isPending ? 'Submitting...' : 'Request Withdrawal'}
        </button>
      </form>

      {withdrawal.isError && (
        <div className="error-message">
          Error: {withdrawal.error instanceof Error ? withdrawal.error.message : 'Unknown error'}
        </div>
      )}

      {withdrawal.isSuccess && withdrawal.data && (
        <div className="success-message">
          <p>Withdrawal requested successfully!</p>
          <p className="withdrawal-id">Withdrawal ID: {withdrawal.data.withdrawal_id}</p>
          <p>Status: {withdrawal.data.status}</p>
          <p>
            Validator Signatures: {withdrawal.data.current_signatures}/{withdrawal.data.required_signatures}
          </p>
        </div>
      )}
    </div>
  )
}
