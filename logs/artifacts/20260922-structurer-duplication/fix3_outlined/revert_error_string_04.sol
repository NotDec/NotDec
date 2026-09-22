contract Decompiled {
    function public_0x3a276523() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.gt */);
        // block_2:
        require(false /* TODO: evm.sgt */);
        // block_3:
        require(false /* TODO: evm.gt10 */);
        // block_4:
        require(!false /* TODO: evm.gt17.not */);
        // block_5:
        require(false /* TODO: evm.gt23 */, "calldata too short");
        // block_6:
        return 0 /* TODO: unresolved value: evm.calldataload9.load */;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }
}
