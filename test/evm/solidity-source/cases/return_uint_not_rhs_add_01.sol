// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract ReturnUintNotRhsAdd01 {
    function notrhsadd(uint256 value, uint256 extra) external pure returns (uint256) {
        return ~(value + extra);
    }
}
