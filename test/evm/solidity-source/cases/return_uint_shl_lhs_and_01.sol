// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract ReturnUintShlLhsAnd01 {
    function shllhsand(uint256 value, uint256 mask, uint256 shift) external pure returns (uint256) {
        return (value & mask) << shift;
    }
}
