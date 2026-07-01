// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolNotEqZeroUint01 {
    function notzero(uint256 value) external pure returns (bool) {
        return !(value == 0);
    }
}
