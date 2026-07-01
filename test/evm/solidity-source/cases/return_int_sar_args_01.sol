// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnIntSarArgs01 {
    function sar(int256 value, uint256 shift) external pure returns (int256) {
        return value >> shift;
    }
}
