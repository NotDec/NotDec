// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract CheckedBoundsEnum {
    enum Choice {
        A,
        B
    }

    function cast(uint256 value) external pure returns (Choice) {
        return Choice(value);
    }
}
