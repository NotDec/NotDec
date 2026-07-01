// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolSleInt01 {
    function issle(int256 left, int256 right) external pure returns (bool) {
        return left <= right;
    }
}
