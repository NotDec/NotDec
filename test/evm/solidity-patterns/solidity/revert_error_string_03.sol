// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract RevertErrorString03 {
    function run(uint256 value) external pure {
        require(value == 0, "value must be zero");
    }
}
