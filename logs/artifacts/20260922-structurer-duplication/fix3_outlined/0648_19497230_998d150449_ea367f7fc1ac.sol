contract Decompiled {
    event Event_0xD9E58D556B4C848B2F5027FB86C540DDBAD746710EADBED608BDE4D994D9A21E(address indexed arg0);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(address indexed arg0, address indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x390B1276974B9463E5D66AB10DF69B6F3D7B930EB066A0E66DF327EDD2CC811C(address indexed arg0, uint256 indexed arg1);
    event Event_0xEC968375D87C8BC8B1FB440238B4E7BC0BABFBBDBA5DC249A1E08CB5E2A19F22(uint256 indexed arg0);
    event Event_0xEBEDB8B3C678666E7F36970BC8F57ABF6D8FA2E828C0DA91EA5B75BF68ED101A(address indexed arg0);
    event Event_0x7F26B83FF96E1F2B6A682F133852F6798A09C465DA95921460CEFB3847402498();
    event Event_0x832BA5C2F240F586467B80C7B5556B58CE0E0684C98D1E95881F9CB74DC51CF7(uint256 indexed arg0);

    uint256[] public slot_1;
    mapping(uint256 => uint256) public slot_101;
    mapping(uint256 => uint256) public slot_102;
    uint256 public slot_103;
    uint256 public slot_104;
    uint256 public slot_105;
    uint256 public slot_151;
    uint256 public slot_152;
    uint256 public slot_153;
    mapping(uint256 => uint256) public slot_154;
    mapping(uint256 => uint256) public slot_155;
    uint256[] public slot_2;
    uint256 public slot_206;
    uint256 public slot_207;
    uint256 public slot_208;
    uint256 public slot_51;

    function MAX_LOCK_DURATION(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return 208;
    }

    function MIN_LOCK_DURATION(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return 4;
    }

    function public_0x2f8ed06b(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
        require(arg1 <= 2);
        // block_6:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function public_0x3f9ffb7d(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        if (arg0 + uint256((type(uint256).max - 208)) < (type(uint256).max - 204)) {
          // block_5:
          revert(); // custom_error_candidate, selector=0x4ede0ebc, args=1
        }
        else {
          // block_6:
          private__0x1f70_0x1f70(uint256(uint160(msg.sender)), 4295);
          require(!false /* TODO: evm.iszero16 */);
          // block_8:
          if (false /* TODO: evm.iszero24 */) {
            // block_9:
            revert(); // custom_error_candidate, selector=0xa6319090
          }
          else {
            // block_10:
            emit Event_0xD9E58D556B4C848B2F5027FB86C540DDBAD746710EADBED608BDE4D994D9A21E(msg.sender); // TODO: recover event signature
          }
        }
    }

    function public_0x44c3ca7e(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        return 0 /* TODO: unresolved value: private__0x22d6_0x22d6 */;
    }

    function public_0x7c993961(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        private__0x1603_0x1603(3613);
    }

    function public_0x8f0cbf8a(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function public_0xbe596d3a(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        if (uint256(uint160(msg.sender)) == slot_152 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // block_6:
          private__0x1f70_0x1f70(arg0, 973);
          emit Event_0xEC968375D87C8BC8B1FB440238B4E7BC0BABFBBDBA5DC249A1E08CB5E2A19F22(arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          return slot_154[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)];
        }
        else {
          // block_5:
          revert(); // custom_error_candidate, selector=0x8a7d9d14, args=0
        }
    }

    function public_0xc89785a3(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_208;
    }

    function public_0xdf9f28c5(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_208;
    }

    function public_0xe8c3a808(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        return 0 /* TODO: unresolved value: private__0x2405_0x2405 */;
    }

    function public_0xf9ab4f90(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_207 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0xfb5cdc2a(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        return 0 /* TODO: unresolved value: private__0x2532_0x2532 */;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function approve(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
        private__0x18bd_0x18bd(uint256(uint160(msg.sender)), uint256(uint160(arg0)), arg1, 257347);
        return 1;
    }

    function balanceOf(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function chi(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_206 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function currentEpoch(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_153;
    }

    function decimals(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return 18;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
        if (false /* TODO: evm.lt */) {
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_6:
          return 1;
        }
    }

    function epochs(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
        return 1;
    }

    function initialize(address arg0, address arg1, address arg2, string calldata arg3, string calldata arg4) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 160);
        // block_4:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        require(uint256(uint160(arg2)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_8:
        require(!false /* TODO: evm.gt */);
        // block_10:
        require(!false /* TODO: evm.gt17 */);
        // block_12:
        if (!false /* TODO: _0x936_0x0.in */) {
          // goto block_13
        }
        // goto block_15
        // block_13:
        if (false /* TODO: _0x93d_0x0 */) {
          revert(); // error_string, selector=0x8c379a0, string_length=46
        }
        // block_15:
        if (false /* TODO: evm.iszero */) {
          // block_16:
        }
        // block_17:
        private__0x165b_0x165b(uint256(uint160(msg.sender)), 2421);
        if (false /* TODO: evm.gt63 */) {
          while (true) {
            // block_18:
            // block_19:
            if (!false /* TODO: evm.lt75 */) {
              break;
            }
            continue;
          }
        }
        // block_21:
        if (!false /* TODO: evm.gt79 */) {
          // goto block_28
        }
        while (true) {
          // block_22:
          // block_23:
          if (!false /* TODO: evm.lt85 */) {
            break;
          }
          continue;
        }
        // block_25:
        if (!false /* TODO: evm.lt94.not */) {
          // goto block_26
        }
        // goto block_27
        // block_28:
        if (!false /* TODO: evm.branch.cond105.not */) {
          // block_26:
        }
        if (false /* TODO: evm.branch.cond105.not */) {
          // goto block_27
        }
        // goto block_26
        // block_27:
        // block_31:
        if (false /* TODO: evm.gt121 */) {
          while (true) {
            // block_32:
            // block_33:
            if (!false /* TODO: evm.lt133 */) {
              break;
            }
            continue;
          }
        }
        // block_35:
        if (false /* TODO: evm.gt137 */) {
          // goto block_36
        }
        // goto block_42
        // block_36:
        while (false /* TODO: evm.lt143 */) {
          // block_38:
        }
        // goto block_41
        // block_39:
        if (!false /* TODO: evm.lt153.not */) {
          // block_40:
        }
        // block_41:
        // goto block_45
        // block_42:
        if (!false /* TODO: evm.branch.cond167.not */) {
          // block_43:
        }
        // block_44:
        // block_45:
        slot_153 = 1;
        if (false /* TODO: evm.iszero */) {
          // block_47:
          emit Event_0x7F26B83FF96E1F2B6A682F133852F6798A09C465DA95921460CEFB3847402498(); // TODO: recover event signature
        }
        else {
          // block_48:
        }
        // block_46:
        // TODO: recover remaining body
    }

    function name(uint256 arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg2);
        // block_4:
        if (!(slot_104 & uint256(1) == 0)) {
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

    function owner(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_51 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function renounceOwnership(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        private__0x1603_0x1603(3730);
        slot_51 = slot_51 & uint256((type(uint256).max - 1461501637330902918203684832716283019655932542975));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_51 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0); // TODO: recover event signature
    }

    function rewardController(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_152 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function stakeToken(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_151 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function stake(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        if (arg0 == 0) {
          // block_5:
          revert(); // custom_error_candidate, selector=0x1f2a2005, args=0
        }
        else {
          // block_6:
          private__0x1f70_0x1f70(uint256(uint160(msg.sender)), 1315);
          if (uint256(uint160(msg.sender)) == 0) {
            // block_7:
            revert(); // error_string, selector=0x8c379a0, string_length=31
          }
          else {
            // block_8:
            slot_103 = private__0x16e7_0x16e7(slot_103, arg0, 1431);
            emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(address(uint160(arg1)), msg.sender); // TODO: recover event signature
            emit Event_0xEBEDB8B3C678666E7F36970BC8F57ABF6D8FA2E828C0DA91EA5B75BF68ED101A(msg.sender); // TODO: recover event signature
          }
        }
    }

    function stakes(address arg0, uint256 arg1) public returns (uint256 ret0, uint256 ret1, uint256 ret2) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        return (0 /* TODO: unresolved value: evm_sload */, slot_1[0 /* TODO: unresolved value: evm_sha3 */], slot_2[0 /* TODO: unresolved value: evm_sha3 */]);
    }

    function symbol(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        if (!(slot_105 & uint256(1) == 0)) {
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

    function totalSupply(uint256 arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        return slot_103;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 96);
        // block_4:
        if (!false /* TODO: evm.branch.cond5.not */) {
          // block_5:
          if (false /* TODO: evm.lt */) {
            // block_6:
            revert(); // error_string, selector=0x8c379a0, string_length=29
          }
          // block_7:
        }
        // block_8:
        private__0x1708_0x1708(uint256(uint160(arg0)), uint256(uint160(arg1)), arg2, 257263);
        return 1;
    }

    function transferOwnership(address arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        private__0x1603_0x1603(557);
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=38
        }
        else {
          // block_6:
          private__0x165b_0x165b(uint256(uint160(arg0)), 256571);
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
        private__0x1708_0x1708(uint256(uint160(msg.sender)), uint256(uint160(arg0)), arg1, 256644);
        return 1;
    }

    function unstake(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
    }

    function unstake(uint256 arg0, address arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
    }

    function updateEpoch(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
        if (uint256(uint160(msg.sender)) == slot_152 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // block_6:
          slot_153 = slot_153 + uint256(1);
          slot_208 = private__0x16e7_0x16e7(slot_208, arg0, 4051);
          emit Event_0x832BA5C2F240F586467B80C7B5556B58CE0E0684C98D1E95881F9CB74DC51CF7(slot_153); // TODO: recover event signature
        }
        else {
          // block_5:
          revert(); // custom_error_candidate, selector=0x8a7d9d14, args=0
        }
    }

    function private__0x1550_0x1550(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x1572_0x1572(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 + uint256(31) & uint256(36893488147419103200)) + uint256(32);
    }

    function private__0x1603_0x1603(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == slot_51 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // block_2:
        }
        else {
          // block_1:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function private__0x165b_0x165b(uint256 arg0, uint256 arg1) internal {
        // block_0:
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_51 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
    }

    function private__0x16e7_0x16e7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x1708_0x1708(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_1:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_2:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=35
          }
          else {
            // block_4:
            if (slot_101[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
              // block_5:
              revert(); // error_string, selector=0x8c379a0, string_length=38
            }
            else {
              // block_6:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(address(uint160(arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max))), address(uint160(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)))); // TODO: recover event signature
            }
          }
        }
    }

    function private__0x18bd_0x18bd(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_1:
          revert(); // error_string, selector=0x8c379a0, string_length=36
        }
        else {
          // block_2:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=34
          }
          else {
            // block_4:
            emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          }
        }
    }

    function private__0x19ed_0x19ed(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (arg0 == 0) {
          // block_1:
          revert(); // error_string, selector=0x8c379a0, string_length=43
        }
        else {
          // block_2:
        }
    }

    function private__0x1a5e_0x1a5e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x1a6b_0x1a6b(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x1f70_0x1f70(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (!(slot_153 == slot_154[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)])) {
          // block_2:
          if (!(slot_154[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
            // block_3:
          }
          // block_4:
        }
        // block_1:
    }
}
