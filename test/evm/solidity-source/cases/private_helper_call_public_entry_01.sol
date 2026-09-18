// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.25;

contract HelperCall {
    uint256 slot_0;
    uint256 slot_1;
    uint256 slot_2;
    uint256 slot_3;

    function sum(uint256 a, uint256 b) internal pure returns (uint256) {
        return a + b;
    }

    function record(uint256 value) internal {
        slot_2 = value;
    }

    function one_store(uint256 word) public {
        slot_3 = sum(1, word);
    }

    function two_stores(uint256 word) public {
        uint256 total = sum(5, word);
        record(total);
        slot_0 = total;
        slot_1 = total;
    }
}
