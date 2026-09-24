contract Decompiled {
    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function run(bytes calldata arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.gt */);
        // block_2:
        require(false /* TODO: evm.sgt */);
        // block_3:
        require(arg0.length < 18446744073709551616);
        // block_4:
        require(!false /* TODO: evm.gt17.not */);
        // block_5:
        require(arg0.length > 3, "calldata too short");
        // block_6:
        return arg0.length;
    }
}
