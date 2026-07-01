// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolRangeUint01 {
    function inrange(uint256 value, uint256 lower, uint256 upper) external pure returns (bool) {
        return lower < value && value < upper;
    }
}
