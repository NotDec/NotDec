contract Decompiled {
    uint256 public slot_0;

    function public_0xd8b30904() public returns (uint256 ret0) {
        // block_0:
        return gasleft();
    }

    function fallback() public {
        // block_0:
    }

    function kill() public {
        // block_0:
        if (false /* TODO: evm.eq */) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
        else {
          // block_1:
        }
        // TODO: recover remaining body
    }
}
