// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolNeUint01 {
    function isne(uint256 left, uint256 right) external pure returns (bool) {
        return left != right;
    }
}
