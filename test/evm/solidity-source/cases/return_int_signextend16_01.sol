// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnIntSignextend1601 {
    function signextend16(uint16 value) external pure returns (int16) {
        return int16(value);
    }
}
