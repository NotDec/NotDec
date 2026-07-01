// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintAddRhsShl01 {
    function addrhsshl(uint256 value, uint256 base, uint256 shift) external pure returns (uint256) {
        return value + (base << shift);
    }
}
