contract Decompiled {
    event Event_0x4DBA622D284ED0014EE4B9A6A68386BE1A4C08A4913AE272DE89199CC686163();
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_12;
    uint256 public slot_13;
    mapping(uint256 => uint256) public slot_14;
    mapping(uint256 => uint256) public slot_15;
    mapping(uint256 => uint256) public slot_16;
    uint256 public slot_17;
    uint256 public slot_18;
    uint256 public slot_19;
    uint256 public slot_2;
    uint256 public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    uint256 public slot_23;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0xa406e5b1() public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return slot_15[0 /* TODO: unresolved value: evm.calldataload.i.load */] != 0;
    }

    function public_0xb05a63ec() public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(false /* TODO: evm.eq.i */);
        // block_4:
        require(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0);
        // block_5:
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x14c6_0x14c6(uint256(uint160(arg0)), 5671);
        private__0x14c6_0x14c6(uint256(uint160(arg1)), 498049);
        return slot_16[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x14c6_0x14c6(uint256(uint160(arg0)), 5369);
        return private__0x4a1_0x4a1(arg1, uint256(uint160(arg0)), 223700) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x14c6_0x14c6(uint256(uint160(arg0)), 497969);
        return slot_14[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function circulatingSupply() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x6c4_0x6c4 */;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 9;
    }

    function enableTrading() public {
        // block_0:
        require(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0);
        // block_1:
    }

    function getOwner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function isOwner(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x14c6_0x14c6(uint256(uint160(arg0)), 497969);
        return (slot_0 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0;
    }

    function maxTransferAmount() public returns (uint256 ret0) {
        // block_0:
        return private__0x738_0x738(224247);
    }

    function maxTxAmount() public returns (uint256 ret0) {
        // block_0:
        return private__0x68b_0x68b(224018);
    }

    function maxWalletSize() public returns (uint256 ret0) {
        // block_0:
        return private__0x6b4_0x6b4(224063);
    }

    function name() public {
        // block_0:
    }

    function removeLimits() public {
        // block_0:
        require(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0);
        // block_1:
        slot_10 = 10000;
        slot_11 = 10000;
        slot_12 = 10000;
    }

    function renounceOwnership() public {
        // block_0:
        require(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0);
        // block_1:
        slot_0 = slot_0 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x4DBA622D284ED0014EE4B9A6A68386BE1A4C08A4913AE272DE89199CC686163(); // TODO: recover event signature
    }

    function setFees(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6) public {
        // block_0:
        require(msg.data.length > 227);
        // block_1:
        private__0x787_0x787(arg6, arg5, arg4, arg3, arg2, arg1, arg0, 224325);
    }

    function setLimits(uint256 arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_3;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x14c6_0x14c6(uint256(uint160(arg0)), 5453);
        private__0x14c6_0x14c6(uint256(uint160(arg1)), 5469);
        private__0x85c_0x85c(private__0xc31_0xc31(arg2, slot_16[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 1495), uint256(uint160(msg.sender)), uint256(uint160(arg0)), 255859);
        return 1;
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x14c6_0x14c6(uint256(uint160(arg0)), 497969);
        require(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0);
        // block_3:
        slot_0 = uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) | slot_0 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x4DBA622D284ED0014EE4B9A6A68386BE1A4C08A4913AE272DE89199CC686163(); // TODO: recover event signature
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x14c6_0x14c6(uint256(uint160(arg0)), 5369);
        return 1;
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function private__0x1010_0x1010(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1694_0x1694(arg1, arg0, 497636);
    }

    function private__0x1238_0x1238(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if ((arg1 ^ slot_2) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_4:
          return slot_17;
        }
        else {
          // block_1:
          if ((arg0 ^ slot_2) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            return slot_18;
          }
          else {
            // block_2:
            return slot_19;
          }
        }
    }

    function private__0x1662_0x1662(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x14c6_0x14c6(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x164b_0x164b(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x4a1_0x4a1(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x85c_0x85c(arg0, arg1, uint256(uint160(msg.sender)), 224436);
        return 1;
    }

    function private__0x1681_0x1681(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x1694_0x1694(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x3e726_0x3e726(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x164b_0x164b(arg0, arg1, 325966);
    }

    function private__0x68b_0x68b(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1662_0x1662(private__0x164b_0x164b(slot_3, slot_12, 1701), slot_4, 294545);
    }

    function private__0x6b4_0x6b4(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1662_0x1662(private__0x164b_0x164b(slot_3, slot_11, 1701), slot_4, 294545);
    }

    function private__0x738_0x738(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1662_0x1662(private__0x164b_0x164b(slot_3, slot_10, 1701), slot_4, 294545);
    }

    function private__0x787_0x787(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6, uint256 arg7) internal {
        // block_0:
        require(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0);
        // block_1:
        slot_22 = arg6;
        slot_21 = arg5;
        slot_17 = arg2;
        slot_23 = arg3;
        slot_19 = arg0;
        slot_18 = arg1;
        slot_20 = arg4;
        require(slot_17 <= private__0x8eb_0x8eb(5, slot_4, 1997));
        // block_3:
        require(slot_18 <= private__0x8eb_0x8eb(5, slot_4, 2023));
        // block_4:
    }

    function private__0x85c_0x85c(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        require(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) != 0);
        // block_1:
        require(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) != 0);
        // block_2:
        emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
    }

    function private__0x8e0_0x8e0(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x164b_0x164b(arg1, arg0, 325966);
    }

    function private__0x8eb_0x8eb(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1662_0x1662(arg1, arg0, 326004);
    }

    function private__0xc31_0xc31(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1681_0x1681(arg1, arg0, 326114);
    }

    function private__0xc3c_0xc3c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (slot_15[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_1:
          if (slot_15[arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
            // block_5:
            require(slot_5 != 0);
            // block_4:
            return 1;
          }
          else {
            // block_3:
            return 1;
          }
        }
        else {
          // block_2:
          return 1;
        }
    }

    function private__0xc8f_0xc8f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if ((slot_2 ^ arg0) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_1:
          if (slot_15[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
            // block_4:
            slot_9 = private__0x1694_0x1694(slot_9, 1, 3293);
            return 1;
          }
          else {
            // block_3:
            return 1;
          }
        }
        else {
          // block_2:
          return 1;
        }
    }

    function private__0xee2_0xee2(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (slot_15[arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_2:
          return slot_15[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0] == 0 ? 1 : 0;
        }
        else {
          // block_1:
          return 0;
        }
    }
}
