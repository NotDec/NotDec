// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintShrArgs01 {
    function shr(uint256 value, uint256 shift) external pure returns (uint256) {
        return value >> shift;
    }
}
