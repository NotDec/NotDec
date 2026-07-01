// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolLtRhsAddUint01 {
    function ltrhsadd(uint256 left, uint256 right, uint256 extra) external pure returns (bool) {
        return left < right + extra;
    }
}
