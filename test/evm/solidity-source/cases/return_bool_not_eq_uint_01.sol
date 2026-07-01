// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolNotEqUint01 {
    function noteq(uint256 left, uint256 right) external pure returns (bool) {
        return !(left == right);
    }
}
