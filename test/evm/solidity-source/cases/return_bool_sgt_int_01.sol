// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolSgtInt01 {
    function issgt(int256 left, int256 right) external pure returns (bool) {
        return left > right;
    }
}
