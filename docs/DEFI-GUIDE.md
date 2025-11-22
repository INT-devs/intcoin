# INTcoin Cross-Chain DeFi Guide

Complete guide to using INTcoin's decentralized finance features across multiple blockchains.

## Table of Contents

- [Overview](#overview)
- [Liquidity Pools](#liquidity-pools)
- [Yield Farming](#yield-farming)
- [Cross-Chain Swaps](#cross-chain-swaps)
- [RPC Commands](#rpc-commands)
- [Examples](#examples)
- [Best Practices](#best-practices)

## Overview

INTcoin's DeFi platform enables trustless cross-chain financial operations including:
- **Liquidity Pools**: Provide liquidity and earn fees
- **Yield Farming**: Stake assets and earn rewards
- **Cross-Chain Swaps**: Exchange assets across different blockchains

All operations use quantum-resistant signatures and HTLC-based atomic swaps for security.

## Liquidity Pools

### What is a Liquidity Pool?

Liquidity pools use an Automated Market Maker (AMM) with the constant product formula:

```
x * y = k
```

Where:
- `x` = Reserve of token A
- `y` = Reserve of token B
- `k` = Constant product

### Adding Liquidity

When you add liquidity, you receive LP (Liquidity Provider) tokens representing your share of the pool.

**Formula**:
```
LP tokens = sqrt(amount_a * amount_b)  // Initial deposit
LP tokens = min(amount_a / reserve_a, amount_b / reserve_b) * total_supply  // Subsequent deposits
```

**RPC Command**:
```bash
intcoin-cli addliquidity "INT/BTC" 100000000 1000000
```

Parameters:
- Pool pair (e.g., "INT/BTC")
- Amount of token A (in base units)
- Amount of token B (in base units)

### Removing Liquidity

Burn LP tokens to withdraw your proportional share of the pool.

**RPC Command**:
```bash
intcoin-cli removeliquidity <position_id> <lp_tokens>
```

### Swapping

Execute a swap with slippage protection.

**Formula**:
```
output = (reserve_out * input_with_fee) / (reserve_in + input_with_fee)
where input_with_fee = input * (1 - fee_rate)
```

**RPC Command**:
```bash
intcoin-cli poolswap "INT/BTC" 100000000 99000000 true
```

Parameters:
- Pool pair
- Input amount
- Minimum output amount (slippage protection)
- Direction (true = A to B, false = B to A)

### Fee Structure

Default trading fee: **0.3%**

Fees are distributed proportionally to all LP token holders.

### Price Impact

Price impact = how much your trade moves the price.

**Formula**:
```
price_before = reserve_out / reserve_in
price_after = new_reserve_out / new_reserve_in
price_impact = ((price_after - price_before) / price_before) * 100
```

**Check Price Impact**:
```bash
intcoin-cli getpriceimpact "INT/BTC" 100000000
```

### Impermanent Loss

Impermanent loss occurs when the price ratio changes after you provide liquidity.

**Formula**:
```
hold_value = 2.0
pool_value = 2 * sqrt(price_ratio_current / price_ratio_initial)
impermanent_loss = ((pool_value - hold_value) / hold_value) * 100
```

**Example**:
- You add 1 BTC + 10,000 INT at 1:10,000 ratio
- Price moves to 1:20,000
- Impermanent loss: ~5.7%
- BUT: You still earn trading fees which can offset this

## Yield Farming

### Overview

Stake your tokens to earn rewards with APY (Annual Percentage Yield).

### Lock Periods and Bonuses

Longer lock periods earn higher APY multipliers:

| Lock Period | APY Multiplier |
|-------------|----------------|
| No lock     | 1.0x (base APY) |
| 1 month     | 1.1x |
| 3 months    | 1.25x |
| 6 months    | 1.5x |
| 1 year      | 2.0x |

**Example**:
- Base APY: 10%
- 1 year lock: 10% * 2.0 = **20% APY**

### Staking

**RPC Command**:
```bash
intcoin-cli stake <chain> <amount> <lock_period_seconds>
```

**Examples**:
```bash
# Stake 1000 INT for 1 year
intcoin-cli stake INTCOIN 100000000000 31536000

# Stake 0.1 BTC for 3 months
intcoin-cli stake BITCOIN 10000000 7776000
```

### Checking Rewards

**RPC Command**:
```bash
intcoin-cli getpendingrewards <stake_id>
```

Returns pending rewards based on time staked and APY.

**Reward Formula**:
```
rewards = amount * apy * (time_staked / 1_year)
where apy = base_apy * lock_period_multiplier
```

### Claiming Rewards

**RPC Command**:
```bash
intcoin-cli claimrewards <stake_id>
```

Claims all pending rewards without unstaking.

### Unstaking

**RPC Command**:
```bash
intcoin-cli unstake <stake_id>
```

**Important**: Cannot unstake before lock period expires. Will return error with time remaining.

### Viewing Stakes

**RPC Command**:
```bash
# Get all your stakes
intcoin-cli liststakes

# Get specific stake info
intcoin-cli getstake <stake_id>
```

## Cross-Chain Swaps

### Overview

Execute trustless atomic swaps across different blockchains using HTLCs.

### Creating a Swap Order

**RPC Command**:
```bash
intcoin-cli createswaporder <from_chain> <to_chain> <from_amount> <min_to_amount> <deadline_timestamp>
```

**Example**:
```bash
# Swap 0.1 BTC to INT
intcoin-cli createswaporder BITCOIN INTCOIN 10000000 99000000000 1735689600
```

Parameters:
- From chain (e.g., BITCOIN, ETHEREUM, LITECOIN, CARDANO)
- To chain
- Amount to send (in source chain's base units)
- Minimum amount to receive (slippage protection)
- Deadline (Unix timestamp)

### Order Status

Orders go through these states:
1. **PENDING**: Waiting for matching
2. **MATCHED**: Counterparty found
3. **EXECUTING**: HTLC created on both chains
4. **COMPLETED**: Swap successful
5. **CANCELLED**: Cancelled before matching
6. **EXPIRED**: Deadline passed

**Check Order**:
```bash
intcoin-cli getswaporder <order_id>
```

### Cancelling an Order

Only possible if status is PENDING.

```bash
intcoin-cli cancelswaporder <order_id>
```

### Fee Estimation

Cross-chain swaps include:
1. Network fees on both chains
2. Bridge fee (~0.5% of swap amount)

**Estimate Fees**:
```bash
intcoin-cli estimateswapfee <chain> <amount>
```

## RPC Commands

### Pool Commands

```bash
# Create a new liquidity pool
intcoin-cli createpool "INT/BTC" 1000000000 100000000

# Get pool information
intcoin-cli getpoolinfo "INT/BTC"

# List all pools
intcoin-cli listpools

# Get your liquidity positions
intcoin-cli getpositions

# Get pool statistics
intcoin-cli getpoolstats "INT/BTC"
```

### Farm Commands

```bash
# Create a yield farm
intcoin-cli createfarm <chain> <base_apy>

# Get farm information
intcoin-cli getfarminfo <chain>

# List all farms
intcoin-cli listfarms

# Add rewards to farm (admin)
intcoin-cli addfarrewards <chain> <amount>
```

### DeFi Manager Commands

```bash
# Get total DeFi statistics
intcoin-cli getdefistats

# Returns:
# - Total liquidity across all pools (USD)
# - Total staked across all farms (USD)
# - Total 24h volume (USD)
# - Number of active pools/farms
# - Number of active swap orders
```

## Examples

### Example 1: Provide INT/BTC Liquidity

```bash
# 1. Create pool (if doesn't exist)
intcoin-cli createpool "INT/BTC" 10000000000000 100000000

# 2. Add liquidity (100,000 INT + 0.01 BTC)
intcoin-cli addliquidity "INT/BTC" 10000000000000 1000000

# 3. Check your position
POSITION_ID=$(intcoin-cli getpositions | jq -r '.[0].position_id')
intcoin-cli getposition $POSITION_ID

# 4. Wait and earn fees...

# 5. Remove liquidity after some time
LP_TOKENS=$(intcoin-cli getposition $POSITION_ID | jq -r '.lp_tokens')
intcoin-cli removeliquidity $POSITION_ID $LP_TOKENS
```

### Example 2: Yield Farming for 6 Months

```bash
# 1. Check available farms
intcoin-cli listfarms

# 2. Stake 1000 INT for 6 months (base APY * 1.5x)
intcoin-cli stake INTCOIN 100000000000 15552000

# 3. Check pending rewards periodically
STAKE_ID=$(intcoin-cli liststakes | jq -r '.[0].stake_id')
intcoin-cli getpendingrewards $STAKE_ID

# 4. Claim rewards (without unstaking)
intcoin-cli claimrewards $STAKE_ID

# 5. After 6 months, unstake
intcoin-cli unstake $STAKE_ID
```

### Example 3: Cross-Chain BTC → ETH Swap

```bash
# 1. Create swap order
# Swap 0.1 BTC to ETH, deadline in 24 hours
DEADLINE=$(($(date +%s) + 86400))
ORDER_ID=$(intcoin-cli createswaporder BITCOIN ETHEREUM 10000000 200000000000000000 $DEADLINE)

# 2. Monitor order status
intcoin-cli getswaporder $ORDER_ID

# 3. Wait for execution...
# The bridge will automatically:
# - Create HTLCs on both chains
# - Execute the atomic swap
# - Update order status to COMPLETED

# 4. Verify completion
intcoin-cli getswaporder $ORDER_ID | jq '.status'
```

## Best Practices

### Liquidity Provision

1. **Balanced Deposits**: Always add liquidity in the pool's current ratio to avoid losing funds
2. **Impermanent Loss Awareness**: Understand IL risk before providing liquidity
3. **Fee Accumulation**: Longer time in pool = more fees earned
4. **Pool Selection**: Higher volume pools generate more fees

### Yield Farming

1. **Lock Period Selection**: 
   - Don't lock if you might need funds
   - Longer locks = higher returns
   - Consider opportunity cost

2. **Compounding**: Regularly claim and re-stake rewards for compound growth

3. **Diversification**: Stake across multiple chains to spread risk

### Cross-Chain Swaps

1. **Slippage Protection**: Always set `min_to_amount` to protect against price changes
2. **Deadline Management**: Set reasonable deadlines (1-24 hours typical)
3. **Fee Awareness**: Account for fees on both chains
4. **Confirmation Times**: Different chains have different confirmation requirements:
   - Bitcoin: ~60 minutes (6 confirmations)
   - Ethereum: ~3 minutes (12 confirmations)
   - Litecoin: ~30 minutes (12 confirmations)
   - Cardano: ~5 minutes (15 confirmations)

### Security

1. **Verify Addresses**: Double-check all addresses before swapping
2. **Test Transactions**: Start with small amounts to test
3. **HTLC Security**: HTLCs automatically refund if swap fails
4. **Quantum-Resistant**: All signatures use Dilithium5

## Troubleshooting

### "Slippage too high" Error

**Cause**: Price moved between order creation and execution.

**Solution**: Increase `min_to_amount` tolerance or wait for better price.

### "Stake still locked" Error

**Cause**: Trying to unstake before lock period expires.

**Solution**: Check remaining time with `getstake <stake_id>` and wait.

### "Insufficient rewards in pool" Error

**Cause**: Farm reward pool is empty.

**Solution**: Wait for admin to add more rewards, or unstake to get principal back.

### Swap Order Expired

**Cause**: Deadline passed before order could be matched/executed.

**Solution**: Create new order with longer deadline.

## Advanced Topics

### Custom Pool Fees

Pool creators can set custom fee rates:

```bash
intcoin-cli setpoolfee "INT/BTC" 0.003  # 0.3%
```

### Pool Rebalancing

Pools automatically rebalance based on trades. Price = reserve_b / reserve_a.

### Flash Loans

Not yet implemented. Coming in future release.

### Liquidity Mining

Additional rewards for providing liquidity to specific pools. Coming in future release.

## Resources

- [Bridge Documentation](BRIDGE-GUIDE.md)
- [Monitoring Guide](BRIDGE-MONITORING.md)
- [RPC API Reference](RPC-API.md)
- [Smart Contracts](../src/contracts/README.md)

## Support

For questions or issues:
- GitHub: https://github.com/intcoin/core
- GitLab: https://gitlab.com/intcoin/crypto
- Email: team@international-coin.org

---

**Lead Developer**: Maddison Lane  
**Last Updated**: November 2025
