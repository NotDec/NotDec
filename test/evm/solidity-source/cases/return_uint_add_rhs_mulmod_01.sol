// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintAddRhsMulmod01 {
    function addrhsmulmod(uint256 base, uint256 left, uint256 right, uint256 modulus) external pure returns (uint256) {
        return base + mulmod(left, right, modulus);
    }
}
