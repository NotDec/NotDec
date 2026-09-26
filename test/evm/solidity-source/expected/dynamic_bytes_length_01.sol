contract Decompiled {
    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function run(bytes calldata arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(__evm_calldataload(4) < 18446744073709551616);
        // block_2:
        require(msg.data.length > __evm_calldataload(4) + uint256(35));
        // block_3:
        require(arg0.length < 18446744073709551616);
        // block_4:
        require(__evm_calldataload(4) + uint256(4) + arg0.length + uint256(32) <= msg.data.length);
        // block_5:
        require(arg0.length > 3, "calldata too short");
        // block_6:
        return arg0.length;
    }

    function __evm_calldataload(uint256 offset) internal pure returns (uint256 value) {
        assembly { value := calldataload(offset) }
    }
}
