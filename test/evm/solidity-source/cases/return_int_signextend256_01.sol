// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnIntSignextend25601 {
    function signextend256(uint256 value) external pure returns (int256) {
        return int256(value);
    }
}
