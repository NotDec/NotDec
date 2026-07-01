// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract RevertErrorString01 {
    function run() external pure {
        require(false, "short");
    }
}
