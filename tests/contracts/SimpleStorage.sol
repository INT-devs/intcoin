// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

/**
 * SimpleStorage - Basic contract for testing IntSC VM
 *
 * This contract demonstrates:
 * - Storage operations (SLOAD, SSTORE)
 * - Function calls
 * - Event emission
 * - View functions
 */
contract SimpleStorage {
    // State variable
    uint256 private storedValue;
    address public owner;

    // Event for value changes
    event ValueChanged(address indexed changer, uint256 oldValue, uint256 newValue);

    constructor(uint256 initialValue) {
        storedValue = initialValue;
        owner = msg.sender;
        emit ValueChanged(address(0), 0, initialValue);
    }

    /**
     * Set the stored value
     * @param newValue The new value to store
     */
    function set(uint256 newValue) public {
        uint256 oldValue = storedValue;
        storedValue = newValue;
        emit ValueChanged(msg.sender, oldValue, newValue);
    }

    /**
     * Get the stored value (view function, no gas cost)
     * @return The current stored value
     */
    function get() public view returns (uint256) {
        return storedValue;
    }

    /**
     * Increment the stored value by 1
     */
    function increment() public {
        uint256 oldValue = storedValue;
        storedValue++;
        emit ValueChanged(msg.sender, oldValue, storedValue);
    }

    /**
     * Decrement the stored value by 1
     */
    function decrement() public {
        require(storedValue > 0, "Value cannot go below zero");
        uint256 oldValue = storedValue;
        storedValue--;
        emit ValueChanged(msg.sender, oldValue, storedValue);
    }

    /**
     * Add to the stored value
     * @param amount Amount to add
     */
    function add(uint256 amount) public {
        uint256 oldValue = storedValue;
        storedValue += amount;
        emit ValueChanged(msg.sender, oldValue, storedValue);
    }
}
