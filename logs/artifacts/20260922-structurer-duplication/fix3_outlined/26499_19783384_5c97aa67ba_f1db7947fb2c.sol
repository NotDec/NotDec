contract Decompiled {
    event Event_0xA5E1F8B4009110F5525798D04AE2125421A12D0590AA52C13682FF1BD3C492CA(address indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_0;

    function public_0x20709efc() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0[0 /* TODO: unresolved value: private__0xa60_0xa60 */] & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0x6d61fe70() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0[uint256(uint160(msg.sender))] == 0) {
          // block_1:
          emit Event_0xA5E1F8B4009110F5525798D04AE2125421A12D0590AA52C13682FF1BD3C492CA(msg.sender, 0 /* TODO: unresolved value: abi_decode_word_from_calldata__0xcce */ >> uint256(96)); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // custom_error_candidate, selector=0x93360fbf, args=1
        }
    }

    function public_0x8a91b0e3() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0[uint256(uint160(msg.sender))] == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0xf91bd6f1, args=1
        }
        else {
          // block_1:
        }
    }

    function public_0x97003203() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(false /* TODO: evm.gt */);
        // block_2:
        require(false /* TODO: evm.slt7 */);
        // block_3:
        return 0 /* TODO: unresolved value: private__0x2ea_0x2ea */;
    }

    function public_0xaacbd72a() public {
        // block_0:
    }

    function public_0xd68f6025() public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
        }
        // block_2:
    }

    function public_0xecd05961() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        return 0 /* TODO: unresolved value: private__0x6b9_0x6b9 */;
    }

    function public_0xf551e2ee() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x6cc_0x6cc */ & uint256(0) - (uint256(1) << uint256(224));
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function isInitialized(address arg0) public returns (uint256 ret0) {
        // block_0:
        return private__0x603_0x603(uint256(uint160(arg0)), 82979);
    }

    function private__0x603_0x603(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] != 0 ? 1 : 0;
    }

    function private__0x6b9_0x6b9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 == 1 || arg0 == 4 ? 1 : 0;
    }

    function private__0xe63_0xe63(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }
}
