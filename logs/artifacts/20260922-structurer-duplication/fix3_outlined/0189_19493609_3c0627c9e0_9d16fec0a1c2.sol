contract Decompiled {
    event Event_0x259EB7B480B3D449F506927269E4665C83C69E4CD797143EAA8F84632DC7A02B();
    event Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x4A39DC06D4C0DBC64B70AF90FD698A233A518AA5D07E595D983B8C0526C8F7FB(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xC3D58168C5AE7397731D063D5BBF3D657854427343F4C083240F7AACAA2D0F62(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);

    mapping(uint256 => uint256) public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    mapping(uint256 => uint256) public slot_12;
    mapping(uint256 => uint256) public slot_13;
    uint256[] public slot_2;
    uint256[] public slot_3;
    mapping(uint256 => uint256) public slot_4;
    uint256[] public slot_5;
    uint256[] public slot_6;
    mapping(uint256 => uint256) public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x122d6a87() public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3, uint256 ret4) {
        // block_0:
    }

    function public_0x1586db9b() public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3, uint256 ret4, uint256 ret5, uint256 ret6, uint256 ret7, uint256 ret8) {
        // block_0:
    }

    function public_0x1e3f27da() public {
        // block_0:
    }

    function public_0x4010ff55() public {
        // block_0:
    }

    function public_0x4e8cdb7e() public {
        // block_0:
    }

    function public_0x862dadf4() public {
        // block_0:
    }

    function public_0xa7c570a5() public {
        // block_0:
    }

    function public_0xc493c32c() public {
        // block_0:
    }

    function public_0xd753eeea() public {
        // block_0:
    }

    function public_0xded0ec12() public {
        // block_0:
    }

    function public_0xfb7b4539() public {
        // block_0:
    }

    function balanceOfBatch(address arg0, uint256 arg1) public {
        // block_0:
    }

    function balanceOf(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        return private__0x726_0x726(arg1, uint256(uint160(arg0)), 540176);
    }

    function exists(uint256 arg0) public returns (bool ret0) {
        // block_0:
        return slot_12[arg0] != 0;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function isApprovedForAll(address arg0, address arg1) public returns (bool ret0) {
        // block_0:
        return private__0x2130_0x2130(uint256(uint160(arg1)), uint256(uint160(arg0)), 890499) != 0;
    }

    function merkleRoot() public returns (uint256 ret0) {
        // block_0:
        return slot_11;
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return slot_3.length & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function renounceOwnership() public {
        // block_0:
        private__0x1081_0x1081(750475);
    }

    function royaltyInfo(uint256 arg0, uint256 arg1) public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        if (!(msg.data.length > 67)) {
          // goto block_4
        }
        // goto block_1
        // block_1:
        if (slot_7[arg0] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
        }
        // block_3:
        return (0 /* TODO: unresolved value: evm.mload31 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0 /* TODO: unresolved value: private__0x3ab6_0x3ab6 */ / uint256(10000));
        // block_4:
        revert(); // empty
    }

    function safeBatchTransferFrom(address arg0, address arg1, uint256 arg2, uint256 arg3, bytes calldata arg4) public {
        // block_0:
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2, uint256 arg3, bytes calldata arg4) public {
        // block_0:
    }

    function setApprovalForAll(address arg0, bool arg1) public {
        // block_0:
    }

    function setMerkleRoot(bytes32 arg0) public {
        // block_0:
        private__0x2407_0x2407(4252);
    }

    function setURI(string calldata arg0) public {
        // block_0:
    }

    function supportsInterface(bytes4 arg0) public returns (bool ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x74d_0x74d */ != 0;
    }

    function symbol() public {
        // block_0:
    }

    function totalMinted(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_12[arg0];
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_5.length;
    }

    function totalSupply(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_12[arg0];
    }

    function transferOwnership(address arg0) public {
        // block_0:
        private__0x21bc_0x21bc(uint256(uint160(arg0)), 890579);
    }

    function uri(uint256 arg0) public {
        // block_0:
    }

    function withdraw() public {
        // block_0:
    }

    function private__0x1081_0x1081(uint256 arg0) internal {
        // block_0:
        private__0x2407_0x2407(4233);
        private__0x257c_0x257c(0, 891216);
    }

    function private__0x14ab_0x14ab(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0xced3e100, args=1
        }
        else {
          // block_1:
          emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
    }

    function private__0x24c3_0x24c3(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
        if (arg3 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x57f447ce, args=1
        }
        else {
          // block_1:
        }
    }

    function private__0x251e_0x251e(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal {
        // block_0:
        if (arg3 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_4:
          revert(); // custom_error_candidate, selector=0x57f447ce, args=1
        }
        else {
          // block_1:
          if (arg4 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // custom_error_candidate, selector=0x1a83514, args=1
          }
          else {
            // block_2:
          }
        }
    }

    function private__0x2130_0x2130(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1];
    }

    function private__0x21bc_0x21bc(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x2407_0x2407(8644);
        if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x1e4fbdf7, args=1
        }
        else {
          // block_1:
          private__0x257c_0x257c(arg0, 892387);
        }
    }

    function private__0x23e3_0x23e3(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(355110957) << uint256(225) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_1:
          return 1;
        }
        else {
          // block_2:
          return private__0x26f9_0x26f9(arg0, 892538);
        }
    }

    function private__0x2407_0x2407(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_3.length) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // custom_error_candidate, selector=0x118cdaa7, args=1
        }
    }

    function private__0x257c_0x257c(uint256 arg0, uint256 arg1) internal {
        // block_0:
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_3.length & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0); // TODO: recover event signature
    }

    function private__0x2676_0x2676(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal {
        // block_0:
        if (arg3 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_4:
          revert(); // custom_error_candidate, selector=0x57f447ce, args=1
        }
        else {
          // block_1:
          if (arg4 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // custom_error_candidate, selector=0x1a83514, args=1
          }
          else {
            // block_2:
          }
        }
    }

    function private__0x26f9_0x26f9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(1826307347) << uint256(225) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_2:
          return 1;
        }
        else {
          // block_1:
          if (uint256(60968199) << uint256(226) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
            // block_3:
            return 1;
          }
          else {
            // block_4:
            return arg0 & uint256(0) - (uint256(1) << uint256(224)) == uint256(33540519) << uint256(224) ? 1 : 0;
          }
        }
    }

    function private__0x28ac_0x28ac(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
    }

    function private__0x2e7d_0x2e7d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & (uint256(1) << uint256(224)) + uint256(type(uint256).max) == 0);
        // block_1:
    }

    function private__0x3a5c_0x3a5c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x3a6f_0x3a6f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x3a97_0x3a97(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x3ab6_0x3ab6(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x3aec_0x3aec(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x3b23_0x3b23(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x3b72_0x3b72(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x3ba9_0x3ba9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x3bd7_0x3bd7(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x3c05_0x3c05(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x726_0x726(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_0[arg0];
    }

    function private__0x74d_0x74d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x23e3_0x23e3(arg0, 890645);
    }

    function private__0x757_0x757(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x2407_0x2407(1887);
    }
}
