// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnIntSmodArgs01 {
    function smod(int256 a, int256 b) external pure returns (int256) {
        return a % b;
    }
}
