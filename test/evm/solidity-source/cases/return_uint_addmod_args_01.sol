// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintAddmodArgs01 {
    function addmodargs(uint256 left, uint256 right, uint256 modulus) external pure returns (uint256) {
        return addmod(left, right, modulus);
    }
}
