// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

error WithBytes32(bytes32 digest);

contract RevertCustomError05 {
    function run(bytes32 digest) external pure {
        revert WithBytes32(digest);
    }
}
