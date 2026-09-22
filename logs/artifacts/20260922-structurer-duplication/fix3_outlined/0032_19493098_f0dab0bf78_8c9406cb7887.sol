contract Decompiled {
    event Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256 indexed arg0);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256 indexed arg0, bool indexed arg1);

    mapping(uint256 => uint256) public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_12;
    uint256 public slot_13;
    uint256 public slot_14;
    uint256 public slot_15;
    uint256 public slot_16;
    mapping(uint256 => uint256) public slot_17;
    mapping(uint256 => uint256) public slot_18;
    mapping(uint256 => uint256) public slot_19;
    uint256 public slot_2;
    mapping(uint256 => uint256) public slot_20;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_7;
    uint256 public slot_8;

    function public_0x26ededb8() public {
        // block_0:
    }

    function public_0x4d54557f() public {
        // block_0:
    }

    function public_0x9f53c5bf() public {
        // block_0:
    }

    function public_0xa9ddeaa2() public {
        // block_0:
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function uniswapV2Router() public {
        // block_0:
    }

    function addPair(address arg0) public {
        // block_0:
        private__0x12dd_0x12dd(uint256(uint160(arg0)), 1878);
    }

    function allowance(address arg0, address arg1) public {
        // block_0:
    }

    function approve(address arg0, uint256 arg1) public {
        // block_0:
        private__0x1769_0x1769(arg1, uint256(uint160(arg0)), private__0x1761_0x1761(2416), 2429);
    }

    function balanceOf(address arg0) public {
        // block_0:
    }

    function burn(uint256 arg0) public {
        // block_0:
        private__0xb43_0xb43(arg0, 1043);
    }

    function buyFee() public {
        // block_0:
    }

    function decimals() public {
        // block_0:
    }

    function decreaseAllowance(address arg0, uint256 arg1) public {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0x1761_0x1761(4047);
        temp_1 = private__0x141f_0x141f(uint256(uint160(arg0)), temp_0, 4061);
        if (temp_1 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          private__0x1769_0x1769(temp_1 - arg1, uint256(uint160(arg0)), temp_0, 4143);
        }
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
    }

    function increaseAllowance(address arg0, uint256 arg1) public {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1761_0x1761(2839);
        private__0x1769_0x1769(private__0x3540_0x3540(private__0x141f_0x141f(uint256(uint160(arg0)), temp_0, 2857), arg1, 2867), uint256(uint160(arg0)), temp_0, 2872);
    }

    function initialTotalSupply() public {
        // block_0:
    }

    function manualSwap(uint256 arg0) public {
        // block_0:
        private__0x1245_0x1245(arg0, 1837);
    }

    function maxTransactionAmount() public {
        // block_0:
    }

    function maxWallet() public {
        // block_0:
    }

    function name() public {
        // block_0:
    }

    function openTrading() public {
        // block_0:
        private__0x259b_0x259b(5095);
        slot_14 = uint256(256) ** uint256(0) | slot_14 & ~(uint256(256) ** uint256(0) * uint256(255));
        slot_14 = uint256(256) ** uint256(1) | slot_14 & ~(uint256(256) ** uint256(1) * uint256(255));
    }

    function owner() public {
        // block_0:
    }

    function removeStuckToken(address arg0) public {
        // block_0:
    }

    function renounceOwnership() public {
        // block_0:
        private__0xd94_0xd94(1363);
    }

    function sellFee() public {
        // block_0:
    }

    function setAutomatedMarketMakerPair(address arg0, bool arg1) public {
        // block_0:
    }

    function setLimits(uint256 arg0, uint256 arg1) public {
        // block_0:
        private__0x259b_0x259b(5003);
    }

    function setMarketingWallet(address arg0) public {
        // block_0:
        require((private__0x1761_0x1761(3285) ^ slot_7 / uint256(256) ** uint256(0)) & uint256(1461501637330902918203684832716283019655932542975) == 0);
        // block_1:
        slot_7 = (uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)) * uint256(256) ** uint256(0) | slot_7 & ~(uint256(256) ** uint256(0) * uint256(1461501637330902918203684832716283019655932542975));
    }

    function setSwapTokensAtAmount(uint256 arg0) public {
        // block_0:
        require((private__0x1761_0x1761(4605) ^ slot_7 / uint256(256) ** uint256(0)) & uint256(1461501637330902918203684832716283019655932542975) == 0);
        // block_1:
    }

    function swapEnabled() public {
        // block_0:
    }

    function swapTokensAtAmount() public {
        // block_0:
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public {
        // block_0:
    }

    function tradingOpen() public {
        // block_0:
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        private__0x1934_0x1934(arg2, private__0x1761_0x1761(2461), uint256(uint160(arg0)), 2474);
    }

    function transferOwnership(address arg0) public {
        // block_0:
        private__0x16c4_0x16c4(uint256(uint160(arg0)), 2171);
    }

    function transfer(address arg0, uint256 arg1) public {
        // block_0:
    }

    function uniswapV2Pair() public {
        // block_0:
    }

    function private__0x2e99_0x2e99(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x2e8f_0x2e8f(arg0, 11938));
        // block_1:
    }

    function private__0x141f_0x141f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_1[arg1 & uint256(1461501637330902918203684832716283019655932542975)];
    }

    function private__0x1245_0x1245(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require((private__0x1761_0x1761(4742) ^ slot_7 / uint256(256) ** uint256(0)) & uint256(1461501637330902918203684832716283019655932542975) == 0);
        // block_1:
    }

    function private__0x12dd_0x12dd(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x259b_0x259b(4837);
        slot_8 = (arg0 & uint256(1461501637330902918203684832716283019655932542975)) * uint256(256) ** uint256(0) | slot_8 & ~(uint256(256) ** uint256(0) * uint256(1461501637330902918203684832716283019655932542975));
        private__0xda8_0xda8(1, slot_8 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975), 4947);
        private__0x26df_0x26df(1, slot_8 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975), 4992);
    }

    function private__0x3137_0x3137(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x3115_0x3115(arg0, 12610);
    }

    function private__0x16c4_0x16c4(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x259b_0x259b(5836);
        if (arg0 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          private__0x2619_0x2619(arg0, 5957);
        }
    }

    function private__0x1761_0x1761(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return uint256(uint160(msg.sender));
    }

    function private__0x1769_0x1769(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            emit Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(arg2 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
          }
        }
    }

    function private__0x1934_0x1934(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x141f_0x141f(arg1, arg2, 6464);
        if (!(temp_0 == type(uint256).max)) {
          // block_2:
          if (temp_0 < arg0) {
            // block_4:
            revert(); // error_string, selector=0x8c379a0
          }
          // block_3:
          private__0x1769_0x1769(temp_0 - arg0, arg1, arg2, 6585);
        }
        // block_1:
    }

    function private__0x26df_0x26df(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        emit Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(arg1 & uint256(1461501637330902918203684832716283019655932542975), arg0 != 0); // TODO: recover event signature
    }

    function private__0x2c96_0x2c96(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x38a4_0x38a4(arg1, arg0, 11428);
    }

    function private__0x2cac_0x2cac(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x392d_0x392d(arg1, arg0, 11450);
    }

    function private__0x2cc2_0x2cc2(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (slot_17[arg0 & uint256(1461501637330902918203684832716283019655932542975)] > block.number) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x2d6d_0x2d6d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x23cd_0x23cd(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          private__0x89bed_0x89bed(arg0, 0, arg1, 9289);
          if (slot_0[arg1 & uint256(1461501637330902918203684832716283019655932542975)] < arg0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            slot_2 = slot_2 - arg0;
            emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg1 & uint256(1461501637330902918203684832716283019655932542975), 0); // TODO: recover event signature
            private__0x89c11_0x89c11(arg0, 0, arg1, 9622);
          }
        }
    }

    function private__0x259b_0x259b(uint256 arg0) internal {
        // block_0:
        if (private__0xe0b_0xe0b(9665) == private__0x1761_0x1761(9635) & uint256(1461501637330902918203684832716283019655932542975)) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x2619_0x2619(uint256 arg0, uint256 arg1) internal {
        // block_0:
        slot_5 = (arg0 & uint256(1461501637330902918203684832716283019655932542975)) * uint256(256) ** uint256(0) | slot_5 & ~(uint256(256) ** uint256(0) * uint256(1461501637330902918203684832716283019655932542975));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975), arg0 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
    }

    function private__0x38a4_0x38a4(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0x2e8f_0x2e8f(arg0, 14511);
        temp_1 = private__0x2e8f_0x2e8f(arg1, 14522);
        return temp_0 * temp_1;
    }

    function private__0x392d_0x392d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2e8f_0x2e8f(arg1, 14659);
        return private__0x2e8f_0x2e8f(arg0, 14648) / temp_0;
    }

    function private__0x39ec_0x39ec(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 16, 14806);
        return temp_0 + uint256(32);
    }

    function private__0x3a0c_0x3a0c(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0x30e1_0x30e1(arg2, arg0, 14881);
        private__0x2f3b_0x2f3b(arg1, arg0 + uint256(32), 14894);
        return arg0 + uint256(64);
    }

    function private__0x29df_0x29df(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            private__0x89c35_0x89c35(arg0, arg1, arg2, 10954);
            if (slot_0[arg2 & uint256(1461501637330902918203684832716283019655932542975)] < arg0) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0
            }
            else {
              // block_3:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
              private__0x89c59_0x89c59(arg0, arg1, arg2, 11345);
            }
          }
        }
    }

    function private__0x2c57_0x2c57(uint256 arg0) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xd4c_0xd4c(uint256(uint160(address(this))), 11362);
        if (!(temp_0 == 0)) {
          // block_1:
          if (temp_0 > slot_11) {
            // block_2:
          }
          // block_3:
        }
        // block_4:
    }

    function private__0x2db1_0x2db1(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(31) & uint256((type(uint256).max - 31));
    }

    function private__0x2e31_0x2e31(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x2e51_0x2e51(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2e31_0x2e31(arg0, 11868);
    }

    function private__0x2e63_0x2e63(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x2e51_0x2e51(arg0, 11884));
        // block_1:
    }

    function private__0x2e8f_0x2e8f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x2f05_0x2f05(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 != 0 ? 1 : 0;
    }

    function private__0x2f20_0x2f20(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x2f3b_0x2f3b(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x2f4a_0x2f4a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x2f3b_0x2f3b(arg1, arg0, 12127);
        return arg0 + uint256(32);
    }

    function private__0x307d_0x307d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256(255);
    }

    function private__0x3099_0x3099(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x30e1_0x30e1(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x30f0_0x30f0(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x30e1_0x30e1(arg1, arg0, 12549);
        return arg0 + uint256(32);
    }

    function private__0x310b_0x310b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x3115_0x3115(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2e31_0x2e31(private__0x310b_0x310b(private__0x2e31_0x2e31(arg0, 12582), 12587), 12592);
    }

    function private__0x3149_0x3149(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x3137_0x3137(arg0, 12628);
    }

    function private__0x316a_0x316a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x31b2_0x31b2(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x2f05_0x2f05(arg0, 12731));
        // block_1:
    }

    function private__0x34f7_0x34f7(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2e8f_0x2e8f(arg0, 13570);
        return temp_0 + uint256(1);
    }

    function private__0x3540_0x3540(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        temp_0 = private__0x2e8f_0x2e8f(arg0, 13643);
        temp_1 = private__0x2e8f_0x2e8f(arg1, 13654);
        return temp_0 + temp_1;
    }

    function private__0x35e2_0x35e2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 22, 13772);
        return temp_0 + uint256(32);
    }

    function private__0x3674_0x3674(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 57, 13918);
        return temp_0 + uint256(64);
    }

    function private__0x3706_0x3706(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 37, 14064);
        return temp_0 + uint256(64);
    }

    function private__0x3ae9_0x3ae9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 38, 15059);
        return temp_0 + uint256(64);
    }

    function private__0x3b7b_0x3b7b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 36, 15205);
        return temp_0 + uint256(64);
    }

    function private__0x3c0d_0x3c0d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 34, 15351);
        return temp_0 + uint256(64);
    }

    function private__0x3c79_0x3c79(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 29, 15459);
        return temp_0 + uint256(32);
    }

    function private__0x3d0b_0x3d0b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 37, 15605);
        return temp_0 + uint256(64);
    }

    function private__0x3d9d_0x3d9d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 35, 15751);
        return temp_0 + uint256(64);
    }

    function private__0x3e09_0x3e09(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 22, 15859);
        return temp_0 + uint256(32);
    }

    function private__0x3e9b_0x3e9b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 53, 16005);
        return temp_0 + uint256(64);
    }

    function private__0x3f07_0x3f07(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 19, 16113);
        return temp_0 + uint256(32);
    }

    function private__0x3f99_0x3f99(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 54, 16259);
        return temp_0 + uint256(64);
    }

    function private__0x405f_0x405f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 33, 16457);
        return temp_0 + uint256(64);
    }

    function private__0x40f1_0x40f1(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 34, 16603);
        return temp_0 + uint256(64);
    }

    function private__0x415d_0x415d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 32, 16711);
        return temp_0 + uint256(32);
    }

    function private__0x41c9_0x41c9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2e8f_0x2e8f(private__0x310b_0x310b(arg0, 16863), 16868);
    }

    function private__0x4384_0x4384(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 38, 17262);
        return temp_0 + uint256(64);
    }

    function private__0x4416_0x4416(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x2d6d_0x2d6d(arg0 + uint256(32), 36, 17408);
        return temp_0 + uint256(64);
    }

    function private__0x89bed_0x89bed(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x89c11_0x89c11(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x89c35_0x89c35(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x89c59_0x89c59(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0xb03_0xb03(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return 9;
    }

    function private__0xb43_0xb43(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x23cd_0x23cd(arg0, private__0x1761_0x1761(2894), 2900);
    }

    function private__0xd4c_0xd4c(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return slot_0[arg0 & uint256(1461501637330902918203684832716283019655932542975)];
    }

    function private__0xd94_0xd94(uint256 arg0) internal {
        // block_0:
        private__0x259b_0x259b(3484);
        private__0x2619_0x2619(0, 3494);
    }

    function private__0xda8_0xda8(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x259b_0x259b(3504);
    }

    function private__0xe0b_0xe0b(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return slot_5 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0xec7_0xec7(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x259b_0x259b(3791);
        if ((arg1 ^ slot_8 / uint256(256) ** uint256(0)) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          private__0x26df_0x26df(arg0, arg1, 3946);
        }
    }
}
