// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolAndNeZeroUint01 {
    function bothnonzero(uint256 left, uint256 right) external pure returns (bool) {
        return left != 0 && right != 0;
    }
}
