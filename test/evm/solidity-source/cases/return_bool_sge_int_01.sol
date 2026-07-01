// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolSgeInt01 {
    function issge(int256 left, int256 right) external pure returns (bool) {
        return left >= right;
    }
}
