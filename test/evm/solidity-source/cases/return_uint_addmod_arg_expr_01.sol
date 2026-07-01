// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintAddmodArgExpr01 {
    function addmodargexpr(uint256 left, uint256 right, uint256 extra, uint256 modulus) external pure returns (uint256) {
        return addmod(left + right, extra, modulus);
    }
}
