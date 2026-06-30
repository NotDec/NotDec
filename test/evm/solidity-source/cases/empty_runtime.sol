// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract EmptyRuntime {
    constructor() {
        assembly {
            mstore8(0, 0)
            return(0, 1)
        }
    }
}
