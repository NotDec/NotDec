// SPDX-License-Identifier: UNLICENSED
pragma solidity 0.8.26;

contract RevertErrorString04 {
    function run(bytes calldata data) external pure returns (uint256) {
        require(data.length > 3, "calldata too short");
        return data.length;
    }
}
