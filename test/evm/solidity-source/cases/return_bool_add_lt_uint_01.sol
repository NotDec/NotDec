// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.20;

contract ReturnBoolAddLtUint01 {
    function sumlt(uint256 left, uint256 right, uint256 limit) external pure returns (bool) {
        return left + right < limit;
    }
}
