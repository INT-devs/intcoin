// Bridge Statistics Component

import { useBridgeInfo, useBridgeBalance } from '@/hooks/useBridge'

interface BridgeStatsProps {
  address?: string
}

export function BridgeStats({ address }: BridgeStatsProps) {
  const { data: info, isLoading: infoLoading } = useBridgeInfo()
  const { data: balance, isLoading: balanceLoading } = useBridgeBalance(address)

  if (infoLoading) {
    return <div className="stats-loading">Loading bridge info...</div>
  }

  if (!info) {
    return <div className="stats-error">Failed to load bridge info</div>
  }

  return (
    <div className="bridge-stats">
      <h2>Bridge Status</h2>

      <div className="stats-grid">
        <div className="stat-card">
          <div className="stat-label">Bridge Status</div>
          <div className={`stat-value status-${info.status}`}>
            {info.status.toUpperCase()}
          </div>
        </div>

        <div className="stat-card">
          <div className="stat-label">Validators</div>
          <div className="stat-value">
            {info.validators.active} / {info.validators.total}
          </div>
          <div className="stat-sublabel">
            Threshold: {info.validators.threshold}
          </div>
        </div>

        <div className="stat-card">
          <div className="stat-label">Bridge Fee</div>
          <div className="stat-value">
            {(info.fee_basis_points / 100).toFixed(2)}%
          </div>
        </div>

        <div className="stat-card">
          <div className="stat-label">Confirmations</div>
          <div className="stat-value">
            <div>BTC: {info.confirmations.bitcoin}</div>
            <div>ETH: {info.confirmations.ethereum}</div>
            <div>LTC: {info.confirmations.litecoin}</div>
          </div>
        </div>
      </div>

      {address && !balanceLoading && balance && (
        <div className="balance-section">
          <h3>Your Balances</h3>
          <div className="balance-grid">
            <div className="balance-card">
              <div className="balance-token">wBTC</div>
              <div className="balance-amount">
                {formatBalance(balance.balances.wBTC, 8)}
              </div>
            </div>
            <div className="balance-card">
              <div className="balance-token">wETH</div>
              <div className="balance-amount">
                {formatBalance(balance.balances.wETH, 18)}
              </div>
            </div>
            <div className="balance-card">
              <div className="balance-token">wLTC</div>
              <div className="balance-amount">
                {formatBalance(balance.balances.wLTC, 8)}
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}

function formatBalance(balance: string, decimals: number): string {
  const num = BigInt(balance)
  const divisor = BigInt(10 ** decimals)
  const whole = num / divisor
  const fraction = num % divisor
  return `${whole}.${fraction.toString().padStart(decimals, '0').slice(0, 8)}`
}
