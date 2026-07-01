// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintExpLhsExpr01 {
    function explhs(uint256 base, uint256 exponent, uint256 outer) external pure returns (uint256) {
        return (base ** exponent) ** outer;
    }
}
