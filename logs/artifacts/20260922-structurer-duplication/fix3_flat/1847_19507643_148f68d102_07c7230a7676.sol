contract Decompiled {
    event Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x4E26044F7FB3724A9375DEB74974FDC2F4E07F265EFCE87F93CFCED354258487(uint256 indexed arg0);
    event Event_0x2F8788117E7EFF1D82E926EC794901D17C78024A50270940304540A733656F0D(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xF6391F5C32D9C69D2A47EA670B442974B53935D1EDC7FD64EB21E047A839171B(uint256 indexed arg0, uint256 indexed arg1, address indexed arg2);
    event Event_0x5DB9EE0A495BF2E6FF9C91A7834C1BA4FDD244A5E8AA4E537BD38AEAE4B073AA();
    event Event_0x62E78CEA01BEE320CD4E420270B5EA74000D11B0C9F74754EBDBFC544B05A258();
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xA3389242138F35E16A456882B75845C8C6B72970ED65963A34EC6F1CC9BE603(uint256 indexed arg0);
    event Event_0x601B85AA9305DBCFBFC81AA2D4B5126C1CE24AFDD1BAD47D6880E5B6DF081F57();
    event Event_0xE152BCA267BA38F000682405E9883C60A372ACE2D8F54AD39DEEBD5A33BB72EE(uint256 indexed arg0);
    event Event_0x5411E8EBF1636D9E83D5FC4900BF80CBAC82E8790DA2A4C94DB4895E889EEDF6();

    uint256 public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256[] public slot_10;
    mapping(uint256 => uint256) public slot_11;
    uint256 public slot_12;
    mapping(uint256 => uint256) public slot_13;
    uint256 public slot_14;
    uint256 public slot_15;
    uint256 public slot_16;
    uint256 public slot_17;
    uint256 public slot_18;
    mapping(uint256 => uint256) public slot_19;
    uint256 public slot_2;
    mapping(uint256 => uint256) public slot_20;
    uint256 public slot_3;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;
    mapping(uint256 => uint256) public slot_6;
    mapping(uint256 => uint256) public slot_7;
    mapping(uint256 => uint256) public slot_8;
    mapping(uint256 => uint256) public slot_9;

    function DEFAULT_ADMIN_ROLE() public returns (uint256 ret0) {
        // block_0:
        return 0;
    }

    function MINTER_ROLE() public returns (uint256 ret0) {
        // block_0:
        return slot_15;
    }

    function public_0x173cf84a() public {
        // block_0:
        require(msg.data.length > 131);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_3:
        require(false /* TODO: evm.eq.i3.i */);
        // block_6:
    }

    function public_0x48571f23() public returns (uint256 ret0) {
        // block_0:
        return slot_16;
    }

    function public_0x63d1399c() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_3:
        require(false /* TODO: evm.eq.i3.i */);
        // block_6:
        return 0 /* TODO: unresolved value: private__0xebb_0xebb */;
    }

    function public_0x6750bcbc() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (false /* TODO: evm.branch.cond.not */) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=47
        }
        else {
          // block_3:
          return slot_20[0 /* TODO: unresolved value: evm.calldataload.i.load */];
        }
    }

    function public_0x6e7dbfcb() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return slot_20[0 /* TODO: unresolved value: evm.calldataload.i.load */];
    }

    function approve(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        private__0x8ac_0x8ac(arg1, uint256(uint160(arg0)), 412845);
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return private__0x1166_0x1166(uint256(uint160(arg0)), 463787);
    }

    function baseURI() public {
        // block_0:
    }

    function burn(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0xc69_0xc69(arg0, 413336);
    }

    function changeDenominator(uint96 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(arg0 & uint256(0) - (uint256(1) << uint256(96)) == 0);
        // block_4:
        if (false /* TODO: evm.branch.cond.not */) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0, string_length=31
        }
        else {
          // block_5:
          emit Event_0x601B85AA9305DBCFBFC81AA2D4B5126C1CE24AFDD1BAD47D6880E5B6DF081F57(); // TODO: recover event signature
          return (uint256(1) << uint256(96)) + uint256(type(uint256).max) & slot_18 & (uint256(1) << uint256(96)) + uint256(type(uint256).max);
        }
    }

    function denominator() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(96)) + uint256(type(uint256).max) & slot_18 & (uint256(1) << uint256(96)) + uint256(type(uint256).max);
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function getApproved(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return private__0x883_0x883(arg0, 412750) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function getDna(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_4[arg0] == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=47
        }
        else {
          // block_3:
          return slot_19[arg0];
        }
    }

    function getRoleAdmin(bytes32 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return slot_13[0 /* TODO: unresolved value: evm.calldataload.i.load */];
    }

    function grantRole(bytes32 arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
    }

    function hasRole(bytes32 arg0, address arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_13[0 /* TODO: unresolved value: evm.calldataload.i.load */] != 0;
    }

    function indexDna(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return slot_19[arg0];
    }

    function isApprovedForAll(address arg0, address arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        return slot_7[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function name() public {
        // block_0:
    }

    function ownerOf(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return private__0xeb0_0xeb0(arg0, 463520) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function pause() public {
        // block_0:
    }

    function paused() public returns (bool ret0) {
        // block_0:
        return slot_12 != 0;
    }

    function renounceRole(bytes32 arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
    }

    function revokeRole(bytes32 arg0, address arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
    }

    function royaltyInfo(uint256 arg0, uint256 arg1) public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        if (slot_1[arg0] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_3:
        }
        // block_4:
        return (0 /* TODO: unresolved value: evm.mload26 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0 /* TODO: unresolved value: private__0x2a3c_0x2a3c */ / uint256(10000));
    }

    function safeMint(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return private__0x1243_0x1243(arg1, uint256(uint160(arg0)), 513988);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        private__0xc4e_0xc4e(arg2, uint256(uint160(arg1)), uint256(uint160(arg0)), 413303);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2, bytes calldata arg3) public {
        // block_0:
        require(msg.data.length > 131);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_5:
        require(false /* TODO: evm.gt.i */);
        // block_6:
        require(false /* TODO: evm.sgt.i */);
        // block_7:
        require(!false /* TODO: evm.gt12.i.i.not */);
        // block_12:
    }

    function setApprovalForAll(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(false /* TODO: evm.eq.i3.i */);
        // block_6:
    }

    function setBaseURI(string calldata arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.gt.i */);
        // block_2:
        require(false /* TODO: evm.sgt.i */);
        // block_3:
        require(!false /* TODO: evm.gt12.i.i.not */);
        // block_8:
    }

    function setDefaultRoyalty(address arg0, uint96 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(arg1 & uint256(0) - (uint256(1) << uint256(96)) == 0);
        // block_6:
    }

    function setDna(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x76c_0x76c(arg1, arg0, 412633);
    }

    function setRole(bytes32 arg0, address arg1, bool arg2) public {
        // block_0:
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_5
        // block_1:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        if (!false /* TODO: evm.eq.i3.i */) {
          // goto block_4
        }
        // goto block_6
        // block_4:
        revert(); // empty
        // block_5:
        revert(); // empty
        // block_6:
        if (false /* TODO: evm.branch.cond.not */) {
          // goto block_11
        }
        // block_7:
        if (false /* TODO: evm.branch.cond3.not */) {
          // block_9:
        }
        else {
          // block_8:
        }
        // block_10:
        emit Event_0xA3389242138F35E16A456882B75845C8C6B72970ED65963A34EC6F1CC9BE603(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        // block_11:
        revert(); // error_string, selector=0x8c379a0, string_length=31
    }

    function setTokenRoyalty(uint256 arg0, address arg1, uint96 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(arg2 & uint256(0) - (uint256(1) << uint256(96)) == 0);
        // block_6:
    }

    function supportsInterface(bytes4 arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return 0 /* TODO: unresolved value: private__0x6ca_0x6ca */ != 0;
    }

    function symbol() public {
        // block_0:
    }

    function tokenByIndex(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (arg0 < slot_10.length) {
          // block_3:
          return slot_10[arg0];
        }
        else {
          // block_4:
          revert(); // custom_error_candidate, selector=0xa57d13dc, args=2
        }
    }

    function tokenOfOwnerByIndex(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        if (arg1 < private__0x1166_0x1166(uint256(uint160(arg0)), 2725)) {
          // block_5:
          return slot_8[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
        }
        else {
          // block_6:
          revert(); // custom_error_candidate, selector=0xa57d13dc, args=2
        }
    }

    function tokenURI(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_4[arg0] == 0) {
          // block_13:
          revert(); // error_string, selector=0x8c379a0, string_length=47
        }
        // block_3:
        if (false /* TODO: evm.gt.i.not */) {
          // goto block_11
        }
        // goto block_4
        // block_4:
        while (!false /* TODO: evm.branch.cond13.i.i.not */) {
          // block_5:
        }
        // block_6:
        if (slot_17 & uint256(1) == 0) {
          // goto block_7
        }
        // goto block_8
        // block_7:
        // goto block_11
        while (true) {
          // block_8:
          // block_9:
          if (!false /* TODO: evm.lt.i.not */) {
            break;
          }
          continue;
        }
        // block_11:
        // block_14:
        // TODO: recover remaining body
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_10.length;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
    }

    function unpause() public {
        // block_0:
    }

    function private__0x1166_0x1166(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x89c62b64, args=1
        }
        else {
          // block_1:
          return slot_5[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0];
        }
    }

    function private__0x18d8_0x18d8(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (slot_13[arg1] == 0) {
          // block_1:
          return 0;
        }
        else {
          // block_2:
          emit Event_0xF6391F5C32D9C69D2A47EA670B442974B53935D1EDC7FD64EB21E047A839171B(arg1, arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), msg.sender); // TODO: recover event signature
          return 1;
        }
    }

    function private__0x195f_0x195f(uint256 arg0) internal {
        // block_0:
        private__0x1fb5_0x1fb5(6503);
        slot_12 = slot_12 & uint256((type(uint256).max - 255));
        emit Event_0x5DB9EE0A495BF2E6FF9C91A7834C1BA4FDD244A5E8AA4E537BD38AEAE4B073AA(); // TODO: recover event signature
    }

    function private__0x1243_0x1243(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (slot_13[slot_15] == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
        else {
          // block_1:
          slot_14 = slot_14 + uint256(1);
          private__0x15b0_0x15b0(arg0, slot_14, 4816);
          private__0x1ab0_0x1ab0(slot_14, arg1, 720899);
          return slot_14;
        }
    }

    function private__0x12e1_0x12e1(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x5b08ba18, args=1
        }
        else {
          // block_1:
          emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
    }

    function private__0x13d1_0x13d1(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
    }

    function private__0x14c3_0x14c3(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x181d_0x181d(slot_13[arg1], 5342);
        private__0x18d8_0x18d8(arg0, arg1, 721046);
    }

    function private__0x14e8_0x14e8(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(2036718347) << uint256(224) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_1:
          return 1;
        }
        else {
          // block_2:
          return private__0x1d32_0x1d32(arg0, 721120);
        }
    }

    function private__0x15b0_0x15b0(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        emit Event_0x4E26044F7FB3724A9375DEB74974FDC2F4E07F265EFCE87F93CFCED354258487(arg1); // TODO: recover event signature
    }

    function private__0x1601_0x1601(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_4[arg0] == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x7e273289, args=1
        }
        else {
          // block_1:
          return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_4[arg0];
        }
    }

    function private__0x163a_0x163a(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x181d_0x181d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x1f60_0x1f60(uint256(uint160(msg.sender)), arg0, 721269);
    }

    function private__0x182a_0x182a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (slot_13[arg1] == 0) {
          // block_2:
          emit Event_0x2F8788117E7EFF1D82E926EC794901D17C78024A50270940304540A733656F0D(arg1, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender))); // TODO: recover event signature
          return 1;
        }
        else {
          // block_1:
          return 0;
        }
    }

    function private__0x23f1_0x23f1(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(2158778573) << uint256(224) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_3:
          return 1;
        }
        else {
          // block_1:
          if (uint256(1532892063) << uint256(224) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
            // block_2:
            return 1;
          }
          else {
            // block_4:
            return private__0x2495_0x2495(arg0, 722039);
          }
        }
    }

    function private__0x1a73_0x1a73(uint256 arg0) internal {
        // block_0:
        private__0x1e6f_0x1e6f(6779);
        emit Event_0x62E78CEA01BEE320CD4E420270B5EA74000D11B0C9F74754EBDBFC544B05A258(); // TODO: recover event signature
    }

    function private__0x1ab0_0x1ab0(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x1d32_0x1d32(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x20d1_0x20d1(arg0, 721564);
    }

    function private__0x1e6f_0x1e6f(uint256 arg0) internal {
        // block_0:
        if (slot_12 == 0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // custom_error_candidate, selector=0xd93c0665, args=0
        }
    }

    function private__0x1f60_0x1f60(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (slot_13[arg1] == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0xe2517d3f, args=2
        }
        else {
          // block_1:
        }
    }

    function private__0x1fb5_0x1fb5(uint256 arg0) internal {
        // block_0:
        if (slot_12 == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x8dfc202b, args=0
        }
        else {
          // block_1:
        }
    }

    function private__0x1fd8_0x1fd8(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x20d1_0x20d1(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(2014223715) << uint256(224) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_1:
          return 1;
        }
        else {
          // block_2:
          return private__0x23f1_0x23f1(arg0, 721929);
        }
    }

    function private__0x2495_0x2495(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(355110957) << uint256(225) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_1:
          return 1;
        }
        else {
          // block_2:
          return arg0 & uint256(0) - (uint256(1) << uint256(224)) == uint256(33540519) << uint256(224) ? 1 : 0;
        }
    }

    function private__0x254d_0x254d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & (uint256(1) << uint256(224)) + uint256(type(uint256).max) == 0);
        // block_1:
    }

    function private__0x2a3c_0x2a3c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x2ccb_0x2ccb(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 & uint256(255)) - (arg1 & uint256(255));
    }

    function private__0x2ce4_0x2ce4(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return (arg1 & uint256(255)) * (arg0 & uint256(255));
    }

    function private__0x2d07_0x2d07(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x6ca_0x6ca(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x14e8_0x14e8(arg0, 564305);
    }

    function private__0x76c_0x76c(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (slot_13[slot_16] == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=38
        }
        else {
          // block_1:
          private__0x15b0_0x15b0(arg0, arg1, 564377);
        }
    }

    function private__0x883_0x883(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        private__0x1601_0x1601(arg0, 2190);
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6[arg0];
    }

    function private__0x8ac_0x8ac(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x163a_0x163a(uint256(uint160(msg.sender)), arg0, arg1, 564606);
    }

    function private__0xa75_0xa75(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x181d_0x181d(slot_13[arg1], 2704);
        private__0x182a_0x182a(arg0, arg1, 564715);
    }

    function private__0xaff_0xaff(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // block_1:
          private__0x18d8_0x18d8(arg0, arg1, 564752);
        }
        else {
          // block_2:
          revert(); // custom_error_candidate, selector=0x6697b232, args=0
        }
    }

    function private__0xc4e_0xc4e(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0xc69_0xc69(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0xeb0_0xeb0(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1601_0x1601(arg0, 720676);
    }
}
