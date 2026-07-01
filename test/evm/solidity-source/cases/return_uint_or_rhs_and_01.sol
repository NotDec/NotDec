// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintOrRhsAnd01 {
    function orrhsand(uint256 left, uint256 middle, uint256 right) external pure returns (uint256) {
        return left | (middle & right);
    }
}
