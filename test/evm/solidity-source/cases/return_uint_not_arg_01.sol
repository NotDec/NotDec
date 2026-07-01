// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintNotArg01 {
    function bitnot(uint256 value) external pure returns (uint256) {
        return ~value;
    }
}
