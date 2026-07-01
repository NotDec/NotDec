pragma solidity ^0.8.26;

contract BaseFeeChecker {
    function getCurrentBaseFee() public view returns (uint) {
        return block.basefee;
    }
}
