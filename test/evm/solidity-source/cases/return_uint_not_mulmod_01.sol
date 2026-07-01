// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintNotMulmod01 {
    function notmulmod(uint256 left, uint256 right, uint256 modulus) external pure returns (uint256) {
        return ~mulmod(left, right, modulus);
    }
}
