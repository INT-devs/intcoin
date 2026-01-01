// INTcoin Bridge TypeScript Types

export type BridgeChain = 'bitcoin' | 'ethereum' | 'litecoin'
export type WrappedToken = 'wBTC' | 'wETH' | 'wLTC'

export interface BridgeConfig {
  status: 'active' | 'paused'
  validators: {
    total: number
    active: number
    threshold: number
  }
  tokens: TokenInfo[]
  confirmations: {
    bitcoin: number
    ethereum: number
    litecoin: number
  }
  fee_basis_points: number
  min_validator_stake: number
  withdrawal_timeout: number
}

export interface TokenInfo {
  symbol: WrappedToken
  origin_chain: BridgeChain
  total_supply: string
  decimals: number
}

export interface DepositRequest {
  chain: BridgeChain
  token: WrappedToken
  tx_hash: string
  block_number: number
  depositor: string
  recipient: string
  amount: string
}

export interface DepositResponse {
  proof_id: string
  status: 'pending' | 'validated' | 'completed'
  amount: string
  token: WrappedToken
}

export interface WithdrawalRequest {
  chain: BridgeChain
  token: WrappedToken
  destination: string
  amount: string
  signature: string
}

export interface WithdrawalResponse {
  withdrawal_id: string
  status: 'pending' | 'signed' | 'executed'
  amount: string
  token: WrappedToken
  destination_chain: BridgeChain
  required_signatures: number
  current_signatures: number
}

export interface BridgeBalance {
  address: string
  balances: {
    wBTC: string
    wETH: string
    wLTC: string
  }
  total_value_int: string
}

export interface BridgeTransaction {
  type: 'deposit' | 'withdrawal'
  tx_id: string
  token: WrappedToken
  amount: string
  source_chain?: BridgeChain
  destination_chain?: BridgeChain
  timestamp: number
  status: string
  confirmations?: number
  signatures?: number
  required_signatures?: number
}

export interface BridgeTransactionsResponse {
  transactions: BridgeTransaction[]
  total: number
  limit: number
  offset: number
  type_filter: string
}

// RPC Error
export interface RPCError {
  code: number
  message: string
  data?: unknown
}

// RPC Response
export interface RPCResponse<T> {
  result?: T
  error?: RPCError
  id?: number
}
