// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract CheckedBoundsMul {
    function mul(uint256 a, uint256 b) external pure returns (uint256) {
        return a * b;
    }
}
