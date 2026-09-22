contract Decompiled {
    function public_0x166a2ad5() public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.eq */);
        // block_2:
        if (false /* TODO: evm.branch.cond10.not */) {
          // block_4:
          revert(); // custom_error_candidate, selector=0x402379fb, args=1
        }
        else {
          // block_3:
        }
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }
}
