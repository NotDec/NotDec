contract Decompiled {
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(address indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);

    uint256 public slot_1;
    uint256 public slot_12;
    uint256 public slot_13;
    uint256 public slot_14;
    uint256 public slot_15;
    uint256 public slot_16;
    uint256 public slot_17;
    uint256 public slot_18;
    mapping(uint256 => uint256) public slot_2;
    mapping(uint256 => uint256) public slot_3;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256[] public slot_9;

    function DOMAIN_SEPARATOR(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return slot_8;
    }

    function TYPEHASH(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return 47450776128926735419978990682686673674206336810486810496149434299031357075913;
    }

    function public_0x49d2a11e(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(arg0 <= 2);
        // block_6:
        return slot_9[arg0];
    }

    function public_0xc1ea07f5(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        private__0x1362_0x1362(1281);
        // block_7:
        revert(); // error_string, selector=0x8c379a0, string_length=23
        if (!(slot_17 == 0)) {
          // goto block_7
        }
        while (false /* TODO: evm.lt.not */) {
          // block_10:
          slot_16 = private__0x16dd_0x16dd(slot_16, 1332);
        }
        // block_9:
        // TODO: recover remaining body
    }

    function baseTokenURI(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
    }

    function minted(address arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function approve(address arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        uint256 temp_0;
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        temp_0 = private__0x16a2_0x16a2(arg1, 3861);
        if (uint256(uint160(msg.sender)) == 0) {
          // block_10:
          emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(temp_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg1); // TODO: recover event signature
        }
        // block_7:
        if (temp_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == uint256(uint160(msg.sender))) {
          // goto block_10
        }
        // goto block_8
        // block_8:
        if (!false /* TODO: evm.iszero18 */) {
          // goto block_10
        }
        // goto block_9
        // block_9:
        revert(); // custom_error_candidate, selector=0xa9fbf51f, args=1
    }

    function balanceOf(address arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_7:
          revert(); // custom_error_candidate, selector=0x89c62b64, args=1
        }
        else {
          // block_8:
          return 0 /* TODO: unresolved value: evm_sload */;
        }
    }

    function currentStage(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return slot_12;
    }

    function getApproved(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        private__0x16a2_0x16a2(arg0, 4082);
        return 0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function isApprovedForAll(address arg0, address arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_8:
        return 0 /* TODO: unresolved value: evm_sload */ & uint256(255) != 0 ? 1 : 0;
    }

    function mint(bytes calldata arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(!false /* TODO: evm.gt */);
        // block_6:
        require(false /* TODO: evm.slt6.not */);
        // block_8:
        require(!false /* TODO: evm.gt12 */);
        // block_10:
        require(!false /* TODO: evm.gt18 */);
        // block_12:
        require(uint256(uint160(tx.origin)) == uint256(uint160(msg.sender)), "Only EOA");
        // block_14:
        if (slot_17 == 0) {
          // block_15:
          if (slot_7 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_16:
            revert(); // error_string, selector=0x8c379a0, string_length=21
          }
          // block_17:
          require(!false /* TODO: evm.gt.i */);
          // block_19:
          if (!false /* TODO: evm.branch.cond74.not */) {
            // block_20:
            revert(); // error_string, selector=0x8c379a0, string_length=17
          }
          // block_21:
        }
        // block_22:
        if (slot_13 > slot_16) {
          // goto block_24
        }
        // block_23:
        revert(); // error_string, selector=0x8c379a0, string_length=38
        // block_24:
        if (false /* TODO: evm.gt98.not */) {
          // block_26:
          slot_16 = private__0x16dd_0x16dd(slot_16, 2189);
        }
        else {
          // block_25:
          revert(); // error_string, selector=0x8c379a0, string_length=27
        }
        // TODO: recover remaining body
    }

    function name(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        if (!false /* TODO: evm.branch.cond3.not.not */) {
          // goto block_6
        }
        // block_5:
        // goto block_9
        // block_6:
        while (false /* TODO: evm.lt */) {
          // block_8:
        }
        // block_9:
        // TODO: recover remaining body
    }

    function ownerOf(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        return private__0x16a2_0x16a2(arg0, 3040) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function owner(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function publicSale(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return slot_17 != 0 ? 1 : 0;
    }

    function renounceOwnership(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        private__0x1362_0x1362(2783);
        slot_6 = slot_6 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg3); // TODO: recover event signature
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length >= 100);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_8:
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2, bytes calldata arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 128);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_8:
        require(!false /* TODO: evm.gt */);
        // block_10:
        require(false /* TODO: evm.slt7.not */);
        // block_12:
        require(!false /* TODO: evm.gt.i */);
        // block_14:
    }

    function setApprovalForAll(address arg0, bool arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(false /* TODO: evm.branch.cond5.not */);
        // block_8:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == 0) {
          // block_9:
          revert(); // custom_error_candidate, selector=0x5b08ba18, args=1
        }
        else {
          // block_10:
          emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(msg.sender, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))); // TODO: recover event signature
        }
    }

    function setBaseURI(string calldata arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        if (!false /* TODO: evm.branch.cond.not */) {
          // goto block_1
        }
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(!false /* TODO: evm.gt */);
        // block_6:
        require(false /* TODO: evm.slt6.not */);
        // block_8:
        require(!false /* TODO: evm.gt.i */);
        // block_10:
        private__0x1362_0x1362(3219);
        if (false /* TODO: evm.gt18 */) {
          while (true) {
            // block_11:
            // block_12:
            if (!false /* TODO: evm.lt28 */) {
              break;
            }
            continue;
          }
        }
        // block_14:
        if (!false /* TODO: evm.gt32 */) {
          // goto block_22
        }
        while (true) {
          // block_15:
          // block_16:
          if (!false /* TODO: evm.lt36 */) {
            break;
          }
          continue;
        }
        // block_18:
        if (!false /* TODO: evm.lt44.not */) {
          // goto block_19
        }
        // goto block_21
        // block_22:
        if (!false /* TODO: evm.branch.cond55.not */) {
          // block_19:
        }
        if (!false /* TODO: evm.branch.cond55.not */) {
          // goto block_19
        }
        // block_21:
        // block_20:
        // TODO: recover remaining body
    }

    function setMintStage(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        private__0x1362_0x1362(2957);
        slot_12 = arg0;
        slot_13 = slot_9[arg0];
    }

    function setPublicSale(bool arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(false /* TODO: evm.branch.cond5.not */);
        // block_6:
        private__0x1362_0x1362(3117);
    }

    function setWhitelistSigningAddress(address arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        private__0x1362_0x1362(681);
        slot_7 = slot_7 & uint256(0) - (uint256(1) << uint256(160)) | uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function supportsInterface(bytes4 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(false /* TODO: evm.branch.cond2.not */);
        // block_6:
        if (!false /* TODO: evm.eq */) {
          // block_7:
          if (!false /* TODO: evm.eq8 */) {
            // block_8:
          }
        }
        // block_9:
        return 0 /* TODO: unresolved value: _0x10ec_0x1.in.reload */;
    }

    function symbol(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        if (!(slot_1 & uint256(1) == 0)) {
          // goto block_6
        }
        // block_5:
        // goto block_9
        // block_6:
        while (false /* TODO: evm.lt */) {
          // block_8:
        }
        // block_9:
        // TODO: recover remaining body
    }

    function tokenURI(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        if (!false /* TODO: evm.branch.cond.not */) {
          // goto block_1
        }
        // goto block_2
        // block_1:
        revert(); // empty
        // block_5:
        // block_6:
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        private__0x16a2_0x16a2(arg0, 739);
        if (false /* TODO: evm.iszero */) {
          // goto block_5
        }
        // goto block_7
        // block_7:
        if (arg0 > 9999999999999999999999999999999999999999999999999999999999999999) {
          // block_8:
        }
        // block_9:
        // block_11:
        if (false /* TODO: evm.lt21 */) {
          // block_12:
        }
        // block_13:
        // block_15:
        if (false /* TODO: evm.lt37 */) {
          // block_16:
        }
        // block_17:
        // block_18:
        // goto block_19
        // block_19:
        while (!false /* TODO: evm.iszero69 */) {
          // block_20:
        }
        // TODO: recover remaining body
    }

    function totalSupply(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg3);
        // block_4:
        return slot_16;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length >= 100);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_8:
    }

    function transferOwnership(address arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        private__0x1362_0x1362(399);
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_7:
          revert(); // custom_error_candidate, selector=0x1e4fbdf7, args=1
        }
        else {
          // block_8:
          slot_6 = slot_6 & uint256(0) - (uint256(1) << uint256(160)) | uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
    }

    function private__0x11f5_0x11f5(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x1217_0x1217(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 + uint256(31) & uint256(36893488147419103200)) + uint256(32);
    }

    function private__0x11c3_0x11c3(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0x16a2_0x16a2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (slot_2[arg0] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_1:
          revert(); // custom_error_candidate, selector=0x7e273289, args=1
        }
        else {
          // block_2:
          return slot_2[arg0] & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
        }
    }

    function private__0x1362_0x1362(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // block_2:
        }
        else {
          // block_1:
          revert(); // custom_error_candidate, selector=0x118cdaa7, args=1
        }
    }

    function private__0x16dd_0x16dd(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }
}
