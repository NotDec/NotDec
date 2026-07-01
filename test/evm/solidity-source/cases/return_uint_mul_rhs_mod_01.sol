// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintMulRhsMod01 {
    function mulrhsmod(uint256 left, uint256 right, uint256 scale) external pure returns (uint256) {
        return left * (right % scale);
    }
}
