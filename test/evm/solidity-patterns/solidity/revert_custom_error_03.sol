// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

error BadSender(address sender, uint256 value);

contract RevertCustomError03 {
    function run(uint256 value) external view {
        revert BadSender(msg.sender, value);
    }
}
