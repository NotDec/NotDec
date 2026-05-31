// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract CheckedBoundsArray {
    function at(uint256[] memory values, uint256 index)
        external
        pure
        returns (uint256)
    {
        return values[index];
    }
}
