contract Decompiled {
    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function run(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(arg0 == 0, "value must be zero");
        // block_2:
    }
}
