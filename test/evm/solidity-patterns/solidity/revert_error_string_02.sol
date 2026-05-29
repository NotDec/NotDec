// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract RevertErrorString02 {
    function run() external pure {
        revert("longer revert message for solidity codegen");
    }
}
