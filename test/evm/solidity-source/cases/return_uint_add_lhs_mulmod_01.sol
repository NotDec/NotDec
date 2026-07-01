// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintAddLhsMulmod01 {
    function addlhsmulmod(uint256 left, uint256 right, uint256 modulus, uint256 base) external pure returns (uint256) {
        return mulmod(left, right, modulus) + base;
    }
}
