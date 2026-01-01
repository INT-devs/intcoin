// React hooks for INTcoin Bridge

import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query'
import { bridgeAPI } from '@/api/bridge'
import type {
  DepositRequest,
  WithdrawalRequest,
} from '@/types/bridge'

export function useBridgeInfo() {
  return useQuery({
    queryKey: ['bridge', 'info'],
    queryFn: () => bridgeAPI.getBridgeInfo(),
    refetchInterval: 30000, // Refetch every 30 seconds
  })
}

export function useBridgeBalance(address?: string) {
  return useQuery({
    queryKey: ['bridge', 'balance', address],
    queryFn: () => bridgeAPI.getBalance(address),
    enabled: !!address,
    refetchInterval: 10000, // Refetch every 10 seconds
  })
}

export function useBridgeTransactions(
  address?: string,
  type: 'deposit' | 'withdrawal' | 'all' = 'all',
  limit = 100,
  offset = 0
) {
  return useQuery({
    queryKey: ['bridge', 'transactions', address, type, limit, offset],
    queryFn: () => bridgeAPI.getTransactions(address, type, limit, offset),
    refetchInterval: 15000, // Refetch every 15 seconds
  })
}

export function useDeposit() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: (request: DepositRequest) => bridgeAPI.submitDeposit(request),
    onSuccess: () => {
      // Invalidate and refetch
      queryClient.invalidateQueries({ queryKey: ['bridge', 'balance'] })
      queryClient.invalidateQueries({ queryKey: ['bridge', 'transactions'] })
    },
  })
}

export function useWithdrawal() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: (request: WithdrawalRequest) =>
      bridgeAPI.requestWithdrawal(request),
    onSuccess: () => {
      // Invalidate and refetch
      queryClient.invalidateQueries({ queryKey: ['bridge', 'balance'] })
      queryClient.invalidateQueries({ queryKey: ['bridge', 'transactions'] })
    },
  })
}
