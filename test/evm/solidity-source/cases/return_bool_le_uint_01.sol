// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolLeUint01 {
    function isle(uint256 left, uint256 right) external pure returns (bool) {
        return left <= right;
    }
}
