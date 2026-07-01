// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract ReturnUintShlRhsAnd01 {
    function shlrhsand(uint256 value, uint256 shift, uint256 mask) external pure returns (uint256) {
        return value << (shift & mask);
    }
}
