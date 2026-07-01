// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolEqZeroUint01 {
    function iszero(uint256 value) external pure returns (bool) {
        return value == 0;
    }
}
