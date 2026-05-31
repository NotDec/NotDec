// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract CheckedBoundsPop {
    uint256[] private values;

    function popValue() external {
        values.pop();
    }
}
