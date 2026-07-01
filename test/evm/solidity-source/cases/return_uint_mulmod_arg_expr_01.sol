// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintMulmodArgExpr01 {
    function mulmodargexpr(uint256 left, uint256 right, uint256 extra, uint256 modulus) external pure returns (uint256) {
        return mulmod(left * right, extra, modulus);
    }
}
