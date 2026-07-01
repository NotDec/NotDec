// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintXorArgs01 {
    function bitxor(uint256 left, uint256 right) external pure returns (uint256) {
        return left ^ right;
    }
}
