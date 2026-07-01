// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolNeZeroUint01 {
    function isnonzero(uint256 value) external pure returns (bool) {
        return value != 0;
    }
}
