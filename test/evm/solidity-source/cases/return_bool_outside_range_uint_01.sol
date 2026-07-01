// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolOutsideRangeUint01 {
    function outside(uint256 value, uint256 lower, uint256 upper) external pure returns (bool) {
        return value < lower || upper < value;
    }
}
