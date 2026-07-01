// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintExpArgs01 {
    function exp(uint256 base, uint256 exponent) external pure returns (uint256) {
        return base ** exponent;
    }
}
