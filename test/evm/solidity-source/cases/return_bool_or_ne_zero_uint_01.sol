// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolOrNeZeroUint01 {
    function eithernonzero(uint256 left, uint256 right) external pure returns (bool) {
        return left != 0 || right != 0;
    }
}
