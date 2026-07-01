// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnUintShlRhsAdd01 {
    function shlrhsadd(uint256 value, uint256 shift, uint256 extra) external pure returns (uint256) {
        return value << (shift + extra);
    }
}
