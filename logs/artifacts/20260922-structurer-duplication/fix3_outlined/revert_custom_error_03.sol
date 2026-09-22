contract Decompiled {
    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function run(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        revert(); // custom_error_candidate, selector=0xf4729f7, args=2
    }
}
