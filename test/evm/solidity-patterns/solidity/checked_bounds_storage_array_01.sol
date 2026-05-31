// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract CheckedBoundsStorageArray {
    uint256[] private values;

    constructor() {
        values.push(7);
    }

    function at(uint256 index) external view returns (uint256) {
        return values[index];
    }
}
