// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolAddEqUint01 {
    function sumis(uint256 left, uint256 right, uint256 target) external pure returns (bool) {
        return left + right == target;
    }
}
