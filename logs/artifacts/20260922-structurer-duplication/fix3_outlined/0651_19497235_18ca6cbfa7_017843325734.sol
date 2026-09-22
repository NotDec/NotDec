contract Decompiled {
    event Event_0xD9E58D556B4C848B2F5027FB86C540DDBAD746710EADBED608BDE4D994D9A21E(address indexed arg0);
    event Event_0x8054E31ACCD385C74219EAC9C238E4261C2A460523DEB5AAB574BDAF8A5C225B(uint256 indexed arg0);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(address indexed arg0, address indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x390B1276974B9463E5D66AB10DF69B6F3D7B930EB066A0E66DF327EDD2CC811C(address indexed arg0, uint256 indexed arg1);
    event Event_0xCEA85328492488AC28180E0A1DB746B3A2EC9E18A44DB26A9F868ADA6C93CE6(address indexed arg0);
    event Event_0x7F26B83FF96E1F2B6A682F133852F6798A09C465DA95921460CEFB3847402498();
    event Event_0xEC968375D87C8BC8B1FB440238B4E7BC0BABFBBDBA5DC249A1E08CB5E2A19F22(uint256 indexed arg0);
    event Event_0xEBEDB8B3C678666E7F36970BC8F57ABF6D8FA2E828C0DA91EA5B75BF68ED101A(address indexed arg0);

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

    function public_0x3a586112(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 96);
        // block_4:
        if (uint256(uint160(msg.sender)) == slot_152 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // block_6:
          slot_153 = slot_153 + uint256(1);
          slot_208 = private__0x1781_0x1781(slot_208, arg0, 4747);
          emit Event_0x8054E31ACCD385C74219EAC9C238E4261C2A460523DEB5AAB574BDAF8A5C225B(slot_153); // TODO: recover event signature
        }
        else {
          // block_5:
          revert(); // custom_error_candidate, selector=0x8a7d9d14, args=0
        }
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
          private__0x1fb3_0x1fb3(uint256(uint160(msg.sender)), 4191);
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
        return 0 /* TODO: unresolved value: private__0x2292_0x2292 */;
    }

    function public_0x7c993961(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        private__0x165f_0x165f(3790);
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
          private__0x1fb3_0x1fb3(arg0, 2476);
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

    function public_0xdd3c0b95(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= arg1);
        // block_4:
        private__0x1fb3_0x1fb3(uint256(uint160(msg.sender)), 871);
        private__0x1f4b_0x1f4b(slot_151 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(msg.sender)), slot_154[uint256(uint160(msg.sender))], 946);
        emit Event_0xCEA85328492488AC28180E0A1DB746B3A2EC9E18A44DB26A9F868ADA6C93CE6(msg.sender); // TODO: recover event signature
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
        return 0 /* TODO: unresolved value: private__0x23b3_0x23b3 */;
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
        return 0 /* TODO: unresolved value: private__0x24e0_0x24e0 */;
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
        private__0x1957_0x1957(uint256(uint160(msg.sender)), uint256(uint160(arg0)), arg1, 260640);
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

    function initialize(address arg0, address arg1, address arg2) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 96);
        // block_4:
        require(uint256(uint160(arg2)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        if (!false /* TODO: _0x47c_0x0.in */) {
          // goto block_7
        }
        // goto block_9
        // block_7:
        if (false /* TODO: _0x483_0x0 */) {
          revert(); // error_string, selector=0x8c379a0, string_length=46
        }
        // block_9:
        if (false /* TODO: evm.iszero */) {
          // block_10:
        }
        // block_11:
        private__0x16b7_0x16b7(uint256(uint160(msg.sender)), 1211);
        if (false /* TODO: evm.gt48 */) {
          while (true) {
            // block_12:
            // block_13:
            if (!false /* TODO: evm.lt60 */) {
              break;
            }
            continue;
          }
        }
        // block_15:
        if (!false /* TODO: evm.gt64 */) {
          // goto block_22
        }
        while (true) {
          // block_16:
          // block_17:
          if (!false /* TODO: evm.lt70 */) {
            break;
          }
          continue;
        }
        // block_19:
        if (!false /* TODO: evm.lt79.not */) {
          // goto block_20
        }
        // goto block_21
        // block_22:
        if (!false /* TODO: evm.branch.cond90.not */) {
          // block_20:
        }
        if (false /* TODO: evm.branch.cond90.not */) {
          // goto block_21
        }
        // goto block_20
        // block_21:
        // block_25:
        if (false /* TODO: evm.gt106 */) {
          while (true) {
            // block_26:
            // block_27:
            if (!false /* TODO: evm.lt118 */) {
              break;
            }
            continue;
          }
        }
        // block_29:
        if (false /* TODO: evm.gt122 */) {
          // goto block_30
        }
        // goto block_36
        // block_30:
        while (false /* TODO: evm.lt128 */) {
          // block_32:
        }
        // goto block_35
        // block_33:
        if (!false /* TODO: evm.lt138.not */) {
          // block_34:
        }
        // block_35:
        // goto block_39
        // block_36:
        if (!false /* TODO: evm.branch.cond152.not */) {
          // block_37:
        }
        // block_38:
        // block_39:
        slot_153 = 1;
        if (false /* TODO: evm.iszero */) {
          // block_41:
          emit Event_0x7F26B83FF96E1F2B6A682F133852F6798A09C465DA95921460CEFB3847402498(); // TODO: recover event signature
        }
        else {
          // block_42:
        }
        // block_40:
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
        private__0x165f_0x165f(3907);
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
          private__0x1fb3_0x1fb3(uint256(uint160(msg.sender)), 2818);
          if (uint256(uint160(msg.sender)) == 0) {
            // block_7:
            revert(); // error_string, selector=0x8c379a0, string_length=31
          }
          else {
            // block_8:
            slot_103 = private__0x1781_0x1781(slot_103, arg0, 2934);
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
        private__0x17a2_0x17a2(uint256(uint160(arg0)), uint256(uint160(arg1)), arg2, 260521);
        return 1;
    }

    function transferOwnership(address arg0, uint256 arg1) public {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 32);
        // block_4:
        private__0x165f_0x165f(568);
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=38
        }
        else {
          // block_6:
          private__0x16b7_0x16b7(uint256(uint160(arg0)), 259829);
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.branch.cond.not */);
        // block_2:
        require(msg.data.length + uint256((type(uint256).max - 3)) >= 64);
        // block_4:
        private__0x17a2_0x17a2(uint256(uint160(msg.sender)), uint256(uint160(arg0)), arg1, 259974);
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

    function private__0x165f_0x165f(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == slot_51 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)) {
          // block_2:
        }
        else {
          // block_1:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function private__0x16b7_0x16b7(uint256 arg0, uint256 arg1) internal {
        // block_0:
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_51 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
    }

    function private__0x1743_0x1743(uint256 arg0, uint256 arg1) internal {
        // block_0:
    }

    function private__0x175f_0x175f(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x1781_0x1781(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x17a2_0x17a2(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x1957_0x1957(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x1c9c_0x1c9c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x1f4b_0x1f4b(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x1fb3_0x1fb3(uint256 arg0, uint256 arg1) internal {
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

    function private__0x25f4_0x25f4(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (arg0 == 0) {
          // block_1:
          revert(); // error_string, selector=0x8c379a0, string_length=43
        }
        else {
          // block_2:
        }
    }
}
