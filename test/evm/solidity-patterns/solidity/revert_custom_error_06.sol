// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

error WithBool(bool flag);

contract RevertCustomError06 {
    function run(bool flag) external pure {
        if (flag) {
            revert WithBool(flag);
        }
    }
}
