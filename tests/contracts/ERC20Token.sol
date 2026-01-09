// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

/**
 * ERC20Token - Standard ERC-20 implementation for IntSC testing
 *
 * This contract demonstrates:
 * - Token transfers
 * - Allowance mechanism
 * - Event logging
 * - Mapping storage
 * - Complex state management
 */
contract ERC20Token {
    string public name;
    string public symbol;
    uint8 public decimals;
    uint256 public totalSupply;

    mapping(address => uint256) private balances;
    mapping(address => mapping(address => uint256)) private allowances;

    // ERC-20 Events
    event Transfer(address indexed from, address indexed to, uint256 value);
    event Approval(address indexed owner, address indexed spender, uint256 value);

    constructor(
        string memory _name,
        string memory _symbol,
        uint8 _decimals,
        uint256 _initialSupply
    ) {
        name = _name;
        symbol = _symbol;
        decimals = _decimals;
        totalSupply = _initialSupply;
        balances[msg.sender] = _initialSupply;
        emit Transfer(address(0), msg.sender, _initialSupply);
    }

    /**
     * Get balance of an account
     * @param account The address to query
     * @return The balance
     */
    function balanceOf(address account) public view returns (uint256) {
        return balances[account];
    }

    /**
     * Transfer tokens to another address
     * @param to The recipient address
     * @param amount The amount to transfer
     * @return success True if transfer succeeded
     */
    function transfer(address to, uint256 amount) public returns (bool) {
        require(to != address(0), "Transfer to zero address");
        require(balances[msg.sender] >= amount, "Insufficient balance");

        balances[msg.sender] -= amount;
        balances[to] += amount;

        emit Transfer(msg.sender, to, amount);
        return true;
    }

    /**
     * Get allowance for spender
     * @param owner The token owner
     * @param spender The approved spender
     * @return The allowance amount
     */
    function allowance(address owner, address spender) public view returns (uint256) {
        return allowances[owner][spender];
    }

    /**
     * Approve spender to spend tokens
     * @param spender The address to approve
     * @param amount The amount to approve
     * @return success True if approval succeeded
     */
    function approve(address spender, uint256 amount) public returns (bool) {
        require(spender != address(0), "Approve to zero address");

        allowances[msg.sender][spender] = amount;
        emit Approval(msg.sender, spender, amount);
        return true;
    }

    /**
     * Transfer tokens from one address to another using allowance
     * @param from The source address
     * @param to The destination address
     * @param amount The amount to transfer
     * @return success True if transfer succeeded
     */
    function transferFrom(address from, address to, uint256 amount) public returns (bool) {
        require(from != address(0), "Transfer from zero address");
        require(to != address(0), "Transfer to zero address");
        require(balances[from] >= amount, "Insufficient balance");
        require(allowances[from][msg.sender] >= amount, "Insufficient allowance");

        balances[from] -= amount;
        balances[to] += amount;
        allowances[from][msg.sender] -= amount;

        emit Transfer(from, to, amount);
        return true;
    }

    /**
     * Mint new tokens (for testing)
     * @param to The address to receive tokens
     * @param amount The amount to mint
     */
    function mint(address to, uint256 amount) public {
        require(to != address(0), "Mint to zero address");

        totalSupply += amount;
        balances[to] += amount;

        emit Transfer(address(0), to, amount);
    }

    /**
     * Burn tokens (for testing)
     * @param amount The amount to burn
     */
    function burn(uint256 amount) public {
        require(balances[msg.sender] >= amount, "Insufficient balance to burn");

        balances[msg.sender] -= amount;
        totalSupply -= amount;

        emit Transfer(msg.sender, address(0), amount);
    }
}
