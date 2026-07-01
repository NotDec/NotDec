// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintDivRhsExpr01 {
    function divrhs(uint256 numerator, uint256 denominator, uint256 scale) external pure returns (uint256) {
        return numerator / (denominator / scale);
    }
}
