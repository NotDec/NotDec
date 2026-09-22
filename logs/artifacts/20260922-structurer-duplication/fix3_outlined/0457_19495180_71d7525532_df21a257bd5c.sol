contract Decompiled {
    event Event_0x4A39DC06D4C0DBC64B70AF90FD698A233A518AA5D07E595D983B8C0526C8F7FB(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xC3D58168C5AE7397731D063D5BBF3D657854427343F4C083240F7AACAA2D0F62(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(uint256 indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;

    function public_0x8062f732() public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
          emit Event_0xC3D58168C5AE7397731D063D5BBF3D657854427343F4C083240F7AACAA2D0F62(slot_6 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975), slot_6 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975), 0 /* TODO: unresolved value: private__0x251e_0x251e */ & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        }
        // block_2:
    }

    function balances(uint256 arg0, address arg1) public {
        // block_0:
    }

    function balanceOfBatch(address arg0, uint256 arg1) public {
        // block_0:
    }

    function balanceOf(address arg0, uint256 arg1) public {
        // block_0:
    }

    function clear() public {
        // block_0:
        emit Event_0xC3D58168C5AE7397731D063D5BBF3D657854427343F4C083240F7AACAA2D0F62(slot_6 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975), slot_6 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975), 0); // TODO: recover event signature
    }

    function contractURI() public {
        // block_0:
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function fromAddr() public {
        // block_0:
    }

    function isApprovedForAll(address arg0, address arg1) public {
        // block_0:
    }

    function name() public {
        // block_0:
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

    function supportsInterface(bytes4 arg0) public {
        // block_0:
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public {
        // block_0:
    }

    function uri(uint256 arg0) public {
        // block_0:
    }

    function private__0x103c_0x103c(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal {
        // block_0:
        if (arg3 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_4:
          revert(); // custom_error_candidate, selector=0x57f447ce
        }
        else {
          // block_1:
          if (arg4 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_3:
            revert(); // custom_error_candidate, selector=0x1a83514
          }
          else {
            // block_2:
          }
        }
    }

    function private__0x1a90_0x1a90(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 != 0 ? 1 : 0;
    }

    function private__0x1aaa_0x1aaa(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x193c_0x193c(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x194d_0x194d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x193c_0x193c(arg0, 6486));
        // block_1:
    }

    function private__0x1977_0x1977(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x1980_0x1980(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x1977_0x1977(arg0, 6537));
        // block_1:
    }

    function private__0x19e8_0x19e8(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x19f7_0x19f7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x19e8_0x19e8(arg1, arg0, 6666);
        return arg0 + uint256(32);
    }

    function private__0x1a3b_0x1a3b(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(26959946667150639794667015087019630673637144422540572481103610249215) == 0);
        // block_1:
    }

    function private__0x243a_0x243a(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0x22a6_0x22a6(arg2, arg0, 9293);
        private__0x22a6_0x22a6(arg1, arg0 + uint256(32), 9306);
        return arg0 + uint256(64);
    }

    function private__0x2461_0x2461(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0x19e8_0x19e8(arg2, arg0, 9332);
        private__0x19e8_0x19e8(arg1, arg0 + uint256(32), 9345);
        return arg0 + uint256(64);
    }

    function private__0x24be_0x24be(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x24c7_0x24c7(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1977_0x1977(private__0x24be_0x24be(arg0, 9436), 9441);
    }

    function private__0x24e8_0x24e8(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x24f7_0x24f7(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0x24e8_0x24e8(arg2, arg0, 9482);
        private__0x19e8_0x19e8(arg1, arg0 + uint256(32), 9495);
        return arg0 + uint256(64);
    }

    function private__0x2552_0x2552(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1977_0x1977(private__0x24be_0x24be(arg0, 9575), 9580);
    }

    function private__0x2582_0x2582(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0x24e8_0x24e8(arg2, arg0, 9621);
        return arg0 + uint256(64);
    }

    function private__0x25d6_0x25d6(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0x1977_0x1977(arg0, 9696);
        temp_1 = private__0x1977_0x1977(arg1, 9707);
        return temp_0 - temp_1;
    }

    function private__0x1b05_0x1b05(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(31) & uint256((type(uint256).max - 31));
    }

    function private__0x22a6_0x22a6(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x22b5_0x22b5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x22a6_0x22a6(arg1, arg0, 8904);
        return arg0 + uint256(32);
    }

    function private__0x2609_0x2609(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal returns (uint256 ret0) {
        // block_0:
        private__0x22a6_0x22a6(arg4, arg0, 9756);
        private__0x19e8_0x19e8(arg3, arg0 + uint256(32), 9769);
        private__0x19e8_0x19e8(arg2, arg0 + uint256(64), 9782);
        private__0x19e8_0x19e8(arg1, arg0 + uint256(96), 9795);
        return arg0 + uint256(128);
    }

    function private__0x264c_0x264c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0x1977_0x1977(arg0, 9814);
        temp_1 = private__0x1977_0x1977(arg1, 9825);
        return temp_0 + temp_1;
    }

    function private__0x3d9_0x3d9(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_0[arg0];
    }

    function private__0x42e_0x42e(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == (type(uint256).max - 17317791891425441398069102864720316188289832523668724183806499875177766060031) || arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == 6574797573728907879914310858245660639267253899780774834407394597170562727936 || arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == 904250603428552709895185118199468575982109441609966099573332780532423983104 ? 1 : 0;
    }

    function private__0xb9a_0xb9a(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xdb2_0xdb2(2981);
        if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0xced3e100
        }
        else {
          // block_1:
          emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(temp_0 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        }
    }

    function private__0xbf5_0xbf5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_1[arg1 & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255);
    }

    function private__0xdb2_0xdb2(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return uint256(uint160(msg.sender));
    }

    function private__0xdb9_0xdb9(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal {
        // block_0:
        if (arg3 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_4:
          revert(); // custom_error_candidate, selector=0x57f447ce
        }
        else {
          // block_1:
          if (arg4 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_3:
            revert(); // custom_error_candidate, selector=0x1a83514
          }
          else {
            // block_2:
          }
        }
    }
}
