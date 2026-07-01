// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintModArgs01 {
    function mod(uint256 a, uint256 b) external pure returns (uint256) {
        return a % b;
    }
}
