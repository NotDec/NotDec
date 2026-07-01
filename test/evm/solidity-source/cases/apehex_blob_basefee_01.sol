// SPDX-License-Identifier: UNLICENSED

pragma solidity 0.8.25;

contract A {
    function getBlobBaseFeeYul() external view returns (uint256 blobBaseFee) {
        assembly {
            blobBaseFee := blobbasefee()
        } 
    }

    function getBlobBaseFeeSolidity() external view returns (uint256 blobBaseFee) {
        blobBaseFee = block.blobbasefee;
    }
}
