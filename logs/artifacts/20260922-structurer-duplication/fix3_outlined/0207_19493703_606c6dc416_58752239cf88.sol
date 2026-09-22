contract Decompiled {
    event Event_0x1ABEBEA81BFA2637F28358C371278FB15EDE7EA8DD28D2E03B112FF6D936ADA4(uint256 indexed arg0);
    event Event_0x95BA64C95D85E67AC83A0476C4A62AC2CF8AB2D0407545B8C9D79C3EEFA62829(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xCE241D7CA1F669FEE44B6FC00B8EBA2DF3BB514EED0F6F668F8F89096E81ED94(uint256 indexed arg0);
    event Event_0x6F50375045128971C5469D343039BA7B8E30A5B190453737B28BDA6F7A306771(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);

    uint256 public slot_0;
    uint256 public slot_1;
    mapping(uint256 => uint256) public slot_2;

    function ANY() public returns (uint256 ret0) {
        // block_0:
        return type(uint256).max;
    }

    function authority() public returns (uint256 ret0) {
        // block_0:
        return slot_0 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function canCall(address arg0, address arg1, bytes4 arg2) public returns (bool ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x760_0x760 */ != 0;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function forbid(address arg0, address arg1, bytes32 arg2) public {
        // block_0:
    }

    function forbid(bytes32 arg0, bytes32 arg1, bytes32 arg2) public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return slot_1 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function permit(address arg0, address arg1, bytes32 arg2) public {
        // block_0:
    }

    function permit(bytes32 arg0, bytes32 arg1, bytes32 arg2) public {
        // block_0:
    }

    function setAuthority(address arg0) public {
        // block_0:
        require(!false /* TODO: evm.iszero3.not */);
        // block_1:
        slot_0 = (uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)) * uint256(256) ** uint256(0) | slot_0 & ~(uint256(256) ** uint256(0) * uint256(1461501637330902918203684832716283019655932542975));
        emit Event_0x1ABEBEA81BFA2637F28358C371278FB15EDE7EA8DD28D2E03B112FF6D936ADA4(slot_0 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
    }

    function setOwner(address arg0) public {
        // block_0:
        require(!false /* TODO: evm.iszero3.not */);
        // block_1:
        slot_1 = (uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)) * uint256(256) ** uint256(0) | slot_1 & ~(uint256(256) ** uint256(0) * uint256(1461501637330902918203684832716283019655932542975));
        emit Event_0xCE241D7CA1F669FEE44B6FC00B8EBA2DF3BB514EED0F6F668F8F89096E81ED94(slot_1 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
    }

    function private__0x512_0x512(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0xce6_0xce6(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }
}
