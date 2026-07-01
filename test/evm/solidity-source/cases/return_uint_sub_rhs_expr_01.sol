// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintSubRhsExpr01 {
    function subExpr(uint256 a, uint256 b, uint256 c) external pure returns (uint256) {
        return a - (b + c);
    }
}
