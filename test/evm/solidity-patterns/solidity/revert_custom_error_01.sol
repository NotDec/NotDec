// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

error NoArgs();

contract RevertCustomError01 {
    function run() external pure {
        revert NoArgs();
    }
}
