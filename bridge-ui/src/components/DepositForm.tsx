// Bridge Deposit Form Component

import { useState } from 'react'
import { useDeposit } from '@/hooks/useBridge'
import type { BridgeChain, WrappedToken } from '@/types/bridge'

export function DepositForm() {
  const [chain, setChain] = useState<BridgeChain>('bitcoin')
  const [token, setToken] = useState<WrappedToken>('wBTC')
  const [txHash, setTxHash] = useState('')
  const [blockNumber, setBlockNumber] = useState('')
  const [depositor, setDepositor] = useState('')
  const [recipient, setRecipient] = useState('')
  const [amount, setAmount] = useState('')

  const deposit = useDeposit()

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()

    try {
      const result = await deposit.mutateAsync({
        chain,
        token,
        tx_hash: txHash,
        block_number: parseInt(blockNumber),
        depositor,
        recipient,
        amount,
      })

      alert(`Deposit submitted successfully!\nProof ID: ${result.proof_id}`)

      // Reset form
      setTxHash('')
      setBlockNumber('')
      setDepositor('')
      setRecipient('')
      setAmount('')
    } catch (error) {
      alert(`Deposit failed: ${error instanceof Error ? error.message : 'Unknown error'}`)
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
    <div className="deposit-form">
      <h3>Deposit to Bridge</h3>
      <form onSubmit={handleSubmit}>
        <div className="form-group">
          <label htmlFor="chain">Source Chain</label>
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
          <label htmlFor="txHash">Transaction Hash</label>
          <input
            id="txHash"
            type="text"
            placeholder="0x..."
            value={txHash}
            onChange={(e) => setTxHash(e.target.value)}
            required
          />
        </div>

        <div className="form-group">
          <label htmlFor="blockNumber">Block Number</label>
          <input
            id="blockNumber"
            type="number"
            placeholder="100000"
            value={blockNumber}
            onChange={(e) => setBlockNumber(e.target.value)}
            required
          />
        </div>

        <div className="form-group">
          <label htmlFor="depositor">Depositor Address (source chain)</label>
          <input
            id="depositor"
            type="text"
            placeholder="0x... or bc1..."
            value={depositor}
            onChange={(e) => setDepositor(e.target.value)}
            required
          />
        </div>

        <div className="form-group">
          <label htmlFor="recipient">Recipient Address (INTcoin)</label>
          <input
            id="recipient"
            type="text"
            placeholder="int1..."
            value={recipient}
            onChange={(e) => setRecipient(e.target.value)}
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

        <button
          type="submit"
          disabled={deposit.isPending}
          className="btn-primary"
        >
          {deposit.isPending ? 'Submitting...' : 'Submit Deposit Proof'}
        </button>
      </form>

      {deposit.isError && (
        <div className="error-message">
          Error: {deposit.error instanceof Error ? deposit.error.message : 'Unknown error'}
        </div>
      )}

      {deposit.isSuccess && deposit.data && (
        <div className="success-message">
          <p>Deposit proof submitted successfully!</p>
          <p className="proof-id">Proof ID: {deposit.data.proof_id}</p>
          <p>Status: {deposit.data.status}</p>
        </div>
      )}
    </div>
  )
}
