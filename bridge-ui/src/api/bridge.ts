// INTcoin Bridge API Client

import type {
  BridgeConfig,
  BridgeBalance,
  BridgeTransactionsResponse,
  DepositRequest,
  DepositResponse,
  WithdrawalRequest,
  WithdrawalResponse,
  RPCResponse,
} from '@/types/bridge'

const RPC_ENDPOINT = '/api'

class BridgeAPIError extends Error {
  constructor(
    message: string,
    public code?: number,
    public data?: unknown
  ) {
    super(message)
    this.name = 'BridgeAPIError'
  }
}

async function rpcCall<T>(method: string, params: unknown[] = []): Promise<T> {
  const response = await fetch(RPC_ENDPOINT, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({
      jsonrpc: '2.0',
      id: Date.now(),
      method,
      params,
    }),
  })

  if (!response.ok) {
    throw new BridgeAPIError(
      `HTTP error: ${response.status} ${response.statusText}`,
      response.status
    )
  }

  const data: RPCResponse<T> = await response.json()

  if (data.error) {
    throw new BridgeAPIError(
      data.error.message,
      data.error.code,
      data.error.data
    )
  }

  if (!data.result) {
    throw new BridgeAPIError('No result in RPC response')
  }

  return data.result
}

export const bridgeAPI = {
  /**
   * Get bridge configuration and status
   */
  async getBridgeInfo(): Promise<BridgeConfig> {
    return rpcCall<BridgeConfig>('getbridgeinfo')
  },

  /**
   * Submit deposit proof to bridge
   */
  async submitDeposit(request: DepositRequest): Promise<DepositResponse> {
    return rpcCall<DepositResponse>('bridgedeposit', [
      request.chain,
      request.token,
      request.tx_hash,
      request.block_number,
      request.depositor,
      request.recipient,
      request.amount,
    ])
  },

  /**
   * Request withdrawal from bridge
   */
  async requestWithdrawal(request: WithdrawalRequest): Promise<WithdrawalResponse> {
    return rpcCall<WithdrawalResponse>('bridgewithdraw', [
      request.chain,
      request.token,
      request.destination,
      request.amount,
      request.signature,
    ])
  },

  /**
   * Get wrapped token balance for address
   */
  async getBalance(address?: string): Promise<BridgeBalance> {
    const params = address ? [address] : []
    return rpcCall<BridgeBalance>('getbridgebalance', params)
  },

  /**
   * List bridge transactions
   */
  async getTransactions(
    address?: string,
    type: 'deposit' | 'withdrawal' | 'all' = 'all',
    limit = 100,
    offset = 0
  ): Promise<BridgeTransactionsResponse> {
    const params = [address, type, limit, offset].filter(
      (p) => p !== undefined
    )
    return rpcCall<BridgeTransactionsResponse>('listbridgetransactions', params)
  },
}

export { BridgeAPIError }
