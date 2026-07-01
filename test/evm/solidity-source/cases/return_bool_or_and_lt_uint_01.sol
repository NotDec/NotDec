// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolOrAndLtUint01 {
    function mixed(uint256 a, uint256 b, uint256 c, uint256 d, uint256 e, uint256 f) external pure returns (bool) {
        return a < b || c < d && e < f;
    }
}
