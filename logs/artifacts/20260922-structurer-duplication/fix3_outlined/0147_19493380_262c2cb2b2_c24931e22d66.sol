contract Decompiled {
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_12;
    uint256 public slot_13;
    uint256 public slot_14;
    uint256 public slot_15;
    mapping(uint256 => uint256) public slot_16;
    mapping(uint256 => uint256) public slot_17;
    mapping(uint256 => uint256) public slot_18;
    uint256 public slot_19;
    mapping(uint256 => uint256) public slot_2;
    uint256 public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    uint256 public slot_23;
    uint256 public slot_24;
    uint256 public slot_25;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function buyMap(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_17[uint256(uint160(arg0))];
    }

    function maxTxAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_23;
    }

    function maxWalletSize() public returns (uint256 ret0) {
        // block_0:
        return slot_24;
    }

    function swapTokensAtAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_25;
    }

    function allowPreTrading(address arg0) public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_4[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x729_0x729(arg1, uint256(uint160(arg0)), 255433) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x91f_0x91f */;
    }

    function blockBots(address arg0) public {
        // block_0:
    }

    function bots(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_16[uint256(uint160(arg0))] != 0;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 9;
    }

    function excludeMultipleAccountsFromFees(address arg0, bool arg1) public {
        // block_0:
    }

    function manualsend() public {
        // block_0:
    }

    function manualswap() public {
        // block_0:
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function preTrader(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_18[uint256(uint160(arg0))] != 0;
    }

    function removePreTrading(address arg0) public {
        // block_0:
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

    function setFee(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) public {
        // block_0:
        require(msg.data.length > 131);
        // block_1:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_2:
          slot_8 = arg0;
          slot_10 = arg1;
          slot_9 = arg2;
          slot_11 = arg3;
        }
        else {
          // block_3:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setMaxTxnAmount(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_23 = arg0;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setMaxWalletSize(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_24 = arg0;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setMinSwapTokensThreshold(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
          slot_25 = arg0;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setTrading(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function symbol() public {
        // block_0:
    }

    function toggleSwap(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 400000000000000000;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x1b46_0x1b46(uint256(uint160(arg0)), 7377);
        private__0x1b46_0x1b46(uint256(uint160(arg1)), 7393);
        return 1;
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
        return private__0xa99_0xa99(arg1, uint256(uint160(arg0)), 256214) != 0;
    }

    function unblockBot(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_22 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_21 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function private__0x1405_0x1405(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 > arg2) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return arg2 - arg1;
        }
    }

    function private__0x186f_0x186f(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x1fd7_0x1fd7(arg2, arg1, 366056);
        }
    }

    function private__0x193c_0x193c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1f1a_0x1f1a(arg1, arg0, 6473);
        if (temp_0 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=27
        }
        else {
          // block_1:
          return temp_0;
        }
    }

    function private__0x1aae_0x1aae(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 == 0) {
          // block_2:
          return 0;
        }
        else {
          // block_1:
          if (private__0x1fd7_0x1fd7(arg1 * arg0, arg1, 6873) == arg0) {
            // block_3:
            return arg1 * arg0;
          }
          else {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=33
          }
        }
    }

    function private__0x1b46_0x1b46(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x1ea0_0x1ea0(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x1f01_0x1f01(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x1f1a_0x1f1a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x1fd7_0x1fd7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x729_0x729(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xd50_0xd50(arg0, arg1, uint256(uint160(msg.sender)), 256640);
        return 1;
    }

    function private__0xa99_0xa99(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0xd50_0xd50(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=36
        }
        else {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=34
          }
          else {
            // block_2:
            emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          }
        }
    }
}
