contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xD7F28048575EEAD8851D024EAD087913957DFB4FD1A02B4D1573F5352A5A2BE3();
    event Event_0x17BBFB9A6069321B6DED73BD96327C9E6B7212A5CD51FF219CD61370ACAFB561();
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x53726DFCAF90650AA7EB35524F4D3220F07413C8D6CB404CC8C18BF5591BC159();
    event Event_0x80F15E9DBC60884FDB59FB8ED4FC48A9A689E028F055E893ED45CA5BE67C5C85(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xC6B438E6A8A59579CE6A4406CBD203B740E0D47B458AAE6596339BCD40C40D15(uint256 indexed arg0);

    uint256 public slot_0;
    uint256[] public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_14;
    uint256 public slot_15;
    uint256 public slot_16;
    uint256 public slot_17;
    uint256[] public slot_2;
    uint256 public slot_22;
    uint256 public slot_23;
    uint256 public slot_24;
    mapping(uint256 => uint256) public slot_25;
    mapping(uint256 => uint256) public slot_26;
    mapping(uint256 => uint256) public slot_27;
    mapping(uint256 => uint256) public slot_28;
    mapping(uint256 => uint256) public slot_29;
    uint256 public slot_3;
    uint256[] public slot_30;
    uint256 public slot_31;
    uint256 public slot_35;
    uint256 public slot_36;
    uint256 public slot_37;
    uint256 public slot_38;
    uint256 public slot_9;

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_27[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x67f_0x67f(arg1, uint256(uint160(arg0)), 298588) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xd47_0xd47 */;
    }

    function burn(uint256 arg0) public {
        // block_0:
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return slot_11 / uint256(65536) & uint256(255);
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        if (slot_27[uint256(uint160(msg.sender))] < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_1:
          private__0x13c4_0x13c4(private__0x2c7c_0x2c7c(slot_27[uint256(uint160(msg.sender))], arg1, 416580), uint256(uint160(arg0)), uint256(uint160(msg.sender)), 3827);
          return 1;
        }
    }

    function excludeFromReward(address arg0) public {
        // block_0:
    }

    function getUnlockTime() public returns (uint256 ret0) {
        // block_0:
        return slot_2.length;
    }

    function includeInReward(address arg0) public {
        // block_0:
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x93c_0x93c(arg1, uint256(uint160(arg0)), 298835) != 0;
    }

    function isExcludedFromFee(address arg0) public returns (bool ret0) {
        // block_0:
        return private__0xcd0_0xcd0(uint256(uint160(arg0)), 299066) != 0;
    }

    function isExcludedFromReward(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_29[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function lock(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_0 = slot_0 & uint256(0) - (uint256(1) << uint256(160));
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function manager() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_3 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function reflectionFromToken(uint256 arg0, bool arg1) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xb9d_0xb9d */;
    }

    function renounceOwnership() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0); // TODO: recover event signature
          slot_0 = slot_0 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setExcludedFromFee(address arg0, bool arg1) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setPreseableEnabled(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_3) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setRouterAddress(address arg0) public {
        // block_0:
    }

    function setSwapAndLiquifyEnabled(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_3) {
          // block_1:
          emit Event_0x53726DFCAF90650AA7EB35524F4D3220F07413C8D6CB404CC8C18BF5591BC159(); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_14;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x2cff_0x2cff(uint256(uint160(arg0)), 10727);
        private__0x2cff_0x2cff(uint256(uint160(arg1)), 10743);
        if (slot_27[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_3:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        else {
          // block_2:
          private__0x13c4_0x13c4(private__0x2c7c_0x2c7c(slot_27[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], arg2, 357879), uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1857);
          return 1;
        }
    }

    function transferManagement(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_3) {
          // block_1:
          emit Event_0x80F15E9DBC60884FDB59FB8ED4FC48A9A689E028F055E893ED45CA5BE67C5C85(slot_3 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          slot_3 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_3 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function transferOwnership(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_0 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_0 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x100c_0x100c(arg1, uint256(uint160(arg0)), 299473) != 0;
    }

    function unlock() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_1.length) {
          // block_1:
          if (block.timestamp > slot_2.length) {
            // block_2:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_1.length); // TODO: recover event signature
            slot_0 = slot_1.length & (uint256(1) << uint256(160)) + uint256(type(uint256).max) | slot_0 & uint256(0) - (uint256(1) << uint256(160));
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=28
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=44
        }
    }

    function withdrawLockedEth(address arg0) public {
        // block_0:
    }

    function private__0x100c_0x100c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x13c4_0x13c4(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=43
        }
        else {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=41
          }
          else {
            // block_2:
            emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          }
        }
    }

    function private__0x1a75_0x1a75(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (!(slot_29[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_22] == 0)) {
          // block_1:
        }
        // block_2:
        emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_22); // TODO: recover event signature
    }

    function private__0x1bda_0x1bda(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_24;
    }

    function private__0x1d33_0x1d33(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if ((slot_0 ^ arg0) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_1:
          return 1;
        }
        else {
          // block_2:
          return (arg0 ^ slot_22) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0 ? 1 : 0;
        }
    }

    function private__0x1d60_0x1d60(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
        if (slot_3 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
          // block_2:
        }
        // block_1:
    }

    function private__0x217f_0x217f(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (!(arg0 == 0)) {
          // block_1:
          if (slot_3 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
            // block_3:
          }
        }
        // block_2:
    }

    function private__0x270b_0x270b(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2c3d_0x2c3d(private__0x2c5d_0x2c5d(arg3, arg1, 10015), slot_11, 10025);
        slot_15 = private__0x2c7c_0x2c7c(slot_15, private__0x2c5d_0x2c5d(temp_0, arg2, 10039), 10055);
        private__0x2921_0x2921(temp_0, arg0, 475502);
    }

    function private__0x2754_0x2754(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2c3d_0x2c3d(private__0x2c5d_0x2c5d(arg3, arg1, 10088), slot_11, 10098);
        private__0x1a75_0x1a75(private__0x2c5d_0x2c5d(temp_0, arg2, 10112), temp_0, uint256(uint160(address(this))), 10125);
        private__0x2921_0x2921(temp_0, arg0, 475541);
    }

    function private__0x2797_0x2797(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal {
        // block_0:
    }

    function private__0x2871_0x2871(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        private__0x13c4_0x13c4(arg0, arg1, arg2, 475618);
    }

    function private__0x2921_0x2921(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x2b3b_0x2b3b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x2b70_0x2b70(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x2c25_0x2c25(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x2c3d_0x2c3d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x2c5d_0x2c5d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 * arg1;
    }

    function private__0x2c7c_0x2c7c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x2cce_0x2cce(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x2cff_0x2cff(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x67f_0x67f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x13c4_0x13c4(arg0, arg1, uint256(uint160(msg.sender)), 299887);
        return 1;
    }

    function private__0x93c_0x93c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x13c4_0x13c4(private__0x2c25_0x2c25(slot_27[uint256(uint160(msg.sender))], arg0, 416017), arg1, uint256(uint160(msg.sender)), 358025);
        return 1;
    }

    function private__0xcd0_0xcd0(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return slot_28[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)];
    }
}
