// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

error BadValue(uint256 value);

contract RevertCustomError02 {
    function run(uint256 value) external pure {
        revert BadValue(value);
    }
}
