contract Decompiled {
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x335CF3124D4C238E9C1BBAE5AE98A445BAB5DE090DF6B8B94E94CAAB3FFB45F6(address indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xB0442210EF7E89767AF2EEB7F3E41F8A7EFEAF443711463E8D0D54106B082A06(address indexed arg0, uint256 indexed arg1, uint256 indexed arg2);

    uint256 public slot_0;
    mapping(uint256 => uint256) public slot_1;
    mapping(uint256 => uint256) public slot_10;
    mapping(uint256 => uint256) public slot_11;
    mapping(uint256 => uint256) public slot_12;
    uint256 public slot_13;
    uint256 public slot_14;
    uint256 public slot_15;
    uint256 public slot_16;
    uint256 public slot_17;
    uint256 public slot_18;
    uint256 public slot_19;
    uint256[] public slot_2;
    uint256 public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    mapping(uint256 => uint256) public slot_23;
    mapping(uint256 => uint256) public slot_24;
    uint256[] public slot_3;
    uint256[] public slot_4;
    mapping(uint256 => uint256) public slot_5;
    mapping(uint256 => uint256) public slot_6;
    mapping(uint256 => uint256) public slot_8;

    function BalancesMap(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return slot_8[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function MAX_MINT() public returns (uint256 ret0) {
        // block_0:
        return slot_17;
    }

    function MintableAddress() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_22 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function PreMintData(uint256 arg0) public {
        // block_0:
        if (!(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31))) {
          // goto block_7
        }
        while (true) {
          // block_1:
          // block_2:
          if (!false /* TODO: evm.lt20.not */) {
            break;
          }
          continue;
        }
        // goto block_5
        // block_3:
        if (!false /* TODO: evm.iszero27 */) {
          // block_4:
        }
        // block_5:
        // block_7:
        revert(); // empty
    }

    function PrintSeries(uint256 arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return slot_24[arg0] != 0;
    }

    function batchMintOwnersMap(uint256 arg0) public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return (slot_10[arg0], slot_10[arg0], slot_10[arg0], (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_10[arg0]);
    }

    function exists(uint256 arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return 0 /* TODO: unresolved value: private__0x1f06_0x1f06 */ != 0;
    }

    function totalBatches() public returns (uint256 ret0) {
        // block_0:
        return slot_19;
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_18;
    }

    function approve(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 67)) < (type(uint256).max - 63));
        // block_1:
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_3:
          revert(); // error_string, selector=0x8c379a0, string_length=42
        }
        else {
          // block_2:
          return slot_8[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
        }
    }

    function baseURI() public {
        while (true) {
          // block_0:
          // block_1:
          if (!false /* TODO: evm.lt.not */) {
            break;
          }
          continue;
        }
        // goto block_4
        // block_2:
        if (!false /* TODO: evm.iszero10 */) {
          // block_3:
        }
        // block_4:
        // TODO: recover remaining body
    }

    function batchMint(address arg0, uint256 arg1, uint256 arg2) public returns (bool ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 99)) < (type(uint256).max - 95));
        // block_1:
        return 0 /* TODO: unresolved value: private__0x1206_0x1206 */ != 0;
    }

    function createPrintSeries(uint256 arg0, uint256 arg1, string calldata arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        require(msg.data.length + uint256((type(uint256).max - 131)) < (type(uint256).max - 127));
        // block_1:
        require(false /* TODO: evm.gt */);
        // block_2:
        require(!false /* TODO: evm.gt13.not */);
        // block_3:
        require(false /* TODO: evm.or1.not */);
        // block_4:
        if ((private__0x2185_0x2185(6833) ^ slot_0) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_5:
          temp_0 = private__0x2084_0x2084(1, slot_21, 10437);
          slot_21 = temp_0;
          emit Event_0x335CF3124D4C238E9C1BBAE5AE98A445BAB5DE090DF6B8B94E94CAAB3FFB45F6(msg.sender, arg1, arg0); // TODO: recover event signature
          return 1;
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function getApproved(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return 0 /* TODO: unresolved value: private__0x1027_0x1027 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function isApprovedForAll(address arg0, address arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 67)) < (type(uint256).max - 63));
        // block_1:
        return private__0x1dda_0x1dda(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), 569058) != 0;
    }

    function mintSeries(uint256 arg0, address arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 67)) < (type(uint256).max - 63));
        // block_1:
        return 0 /* TODO: unresolved value: private__0xc47_0xc47 */ != 0;
    }

    function mintWithURI(address arg0, string calldata arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 99)) < (type(uint256).max - 95));
        // block_1:
        require(false /* TODO: evm.gt */);
        // block_2:
        require(!false /* TODO: evm.gt13.not */);
        // block_3:
        require(false /* TODO: evm.or1.not */);
        // block_4:
        if ((private__0x2185_0x2185(7056) ^ slot_0) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_5:
          private__0x277a_0x277a(arg2, uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 651961);
          return 1;
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function mint(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 67)) < (type(uint256).max - 63));
        // block_1:
        if ((private__0x2185_0x2185(4922) ^ slot_0) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          private__0x277a_0x277a(arg1, uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 5012);
          return 1;
        }
        else {
          // block_3:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function name() public {
        while (true) {
          // block_0:
          // block_1:
          if (!false /* TODO: evm.lt.not */) {
            break;
          }
          continue;
        }
        // goto block_4
        // block_2:
        if (!false /* TODO: evm.iszero10 */) {
          // block_3:
        }
        // block_4:
        // TODO: recover remaining body
    }

    function ownerOf(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return 0 /* TODO: unresolved value: private__0x1461_0x1461 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function renounceOwnership() public {
        // block_0:
        if ((private__0x2185_0x2185(5944) ^ slot_0) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_1:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0); // TODO: recover event signature
          slot_0 = slot_0 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function royaltyAmounts(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return slot_11[arg0];
    }

    function royaltyInfo(uint256 arg0, uint256 arg1) public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 67)) < (type(uint256).max - 63));
        // block_1:
        return (0 /* TODO: unresolved value: private.ret6 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0 /* TODO: unresolved value: private.ret */);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 99)) < (type(uint256).max - 95));
        // block_1:
        private__0x139d_0x139d(arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), 402941);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2, bytes calldata arg3) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 131)) < (type(uint256).max - 127));
        // block_1:
        require(false /* TODO: evm.gt */);
        // block_2:
        require(!false /* TODO: evm.gt16.not */);
        // block_3:
        require(false /* TODO: evm.or1.not */);
        // block_4:
        if (false /* TODO: evm.branch.cond38.not */) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0, string_length=49
        }
        else {
          // block_5:
        }
    }

    function setApprovalForAll(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 67)) < (type(uint256).max - 63));
        // block_1:
        require((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) != (uint256(1) << uint256(160)) + uint256(type(uint256).max) & private__0x2185_0x2185(6559), "ERC721: approve to caller");
        // block_2:
        emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31((uint256(1) << uint256(160)) + uint256(type(uint256).max) & private__0x2185_0x2185(6738), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
    }

    function setApprovalForExchange(address arg0, address arg1, bool arg2) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 99)) < (type(uint256).max - 95));
        // block_1:
        if ((private__0x2185_0x2185(6262) ^ slot_0) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          require((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) != (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), "ERC721: approve to caller");
          // block_3:
          emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function supportsInterface(bytes4 arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return 0 /* TODO: unresolved value: private__0xc24_0xc24 */ != 0;
    }

    function symbol() public {
        while (true) {
          // block_0:
          // block_1:
          if (!false /* TODO: evm.lt.not */) {
            break;
          }
          continue;
        }
        // goto block_4
        // block_2:
        if (!false /* TODO: evm.iszero10 */) {
          // block_3:
        }
        // block_4:
        // TODO: recover remaining body
    }

    function tokenByIndex(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        return 0 /* TODO: unresolved value: private__0x13cd_0x13cd */;
    }

    function tokenOfOwnerByIndex(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 67)) < (type(uint256).max - 63));
        // block_1:
        return 0 /* TODO: unresolved value: private__0x12c8_0x12c8 */;
    }

    function tokenOwners(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 67)) < (type(uint256).max - 63));
        // block_1:
        return 0 /* TODO: unresolved value: private__0x17d2_0x17d2 */;
    }

    function tokenURI(uint256 arg0) public {
        // block_0:
        if (!(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31))) {
          // goto block_7
        }
        while (true) {
          // block_1:
          // block_2:
          if (!false /* TODO: evm.lt9.not */) {
            break;
          }
          continue;
        }
        // goto block_5
        // block_3:
        if (!false /* TODO: evm.iszero15 */) {
          // block_4:
        }
        // block_5:
        // block_7:
        revert(); // empty
    }

    function totalSeries() public returns (uint256 ret0) {
        // block_0:
        return slot_21;
    }

    function totalSupply_2() public returns (uint256 ret0) {
        // block_0:
        return private__0x1170_0x1170(402610);
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 99)) < (type(uint256).max - 95));
        // block_1:
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length + uint256((type(uint256).max - 35)) < (type(uint256).max - 31));
        // block_1:
        if ((private__0x2185_0x2185(7702) ^ slot_0) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_3:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_0 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_0 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function private__0x1170_0x1170(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return slot_18;
    }

    function private__0x139d_0x139d(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x1dda_0x1dda(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_6[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1];
    }

    function private__0x2e77_0x2e77(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (slot_1[arg1] == 0) {
          // block_1:
          return 0;
        }
        else {
          // block_2:
          return 1;
        }
    }

    function private__0x2084_0x2084(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (arg0 + arg1 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=27
        }
        else {
          // block_1:
          return arg0 + arg1;
        }
    }

    function private__0x20ea_0x20ea(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 == 0) {
          // block_2:
          return 0;
        }
        else {
          // block_1:
          if (arg0 * arg1 / arg1 == arg0) {
            // block_3:
            return arg0 * arg1;
          }
          else {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=33
          }
        }
    }

    function private__0x2185_0x2185(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return uint256(uint160(msg.sender));
    }

    function private__0x277a_0x277a(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
        else {
          // block_1:
          temp_0 = private__0x2084_0x2084(1, slot_18, 10211);
          slot_18 = temp_0;
          private__0x2c37_0x2c37(arg1, slot_18, 3, 10266);
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(0, arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), slot_18); // TODO: recover event signature
        }
    }

    function private__0x289c_0x289c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2cb5_0x2cb5(arg0, arg1, 734642);
    }

    function private__0x2c1f_0x2c1f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2e77_0x2e77(arg0, arg1, 734866);
    }

    function private__0x2c2b_0x2c2b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2f3d_0x2f3d(arg0, arg1, 734904);
    }

    function private__0x2c37_0x2c37(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2f87_0x2f87(arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg1, arg2, 734942);
    }

    function private__0x2cb5_0x2cb5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_1[arg1] != 0 ? 1 : 0;
    }

    function private__0x2f3d_0x2f3d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (private__0x2cb5_0x2cb5(arg0, arg1, 12105) == 0) {
          // block_2:
          return 1;
        }
        else {
          // block_1:
          return 0;
        }
    }

    function private__0x2f87_0x2f87(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (slot_1[arg2] == 0) {
          // block_2:
          return 1;
        }
        else {
          // block_1:
          return 0;
        }
    }

    function private__0xc24_0xc24(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return slot_1[arg0 & uint256(0) - (uint256(1) << uint256(224))];
    }
}
