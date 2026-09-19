// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.25;

contract HelperCall {
    uint256 slot_0;

    function sum(uint256 a, uint256 b) internal pure returns (uint256) {
        return a + b;
    }

    function triple(uint256 a) internal pure returns (uint256) {
        return a * 3;
    }

    function calls_helpers(uint256 word) public {
        slot_0 = triple(sum(5, word));
    }
}
