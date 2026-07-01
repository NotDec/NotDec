// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnIntSignextend801 {
    function signextend8(uint8 value) external pure returns (int8) {
        return int8(value);
    }
}
