// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

error NamedPair(uint256 left, uint256 right);

contract RevertCustomError04 {
    function run(uint256 left, uint256 right) external pure {
        if (left != right) {
            revert NamedPair(left, right);
        }
    }
}
