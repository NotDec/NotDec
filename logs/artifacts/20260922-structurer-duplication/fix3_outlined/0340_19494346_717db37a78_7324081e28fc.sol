contract Decompiled {
    event Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256 indexed arg0, bool indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x90B8024C4923D3873FF5B9FCB43D0360D4B9217FA41225D07BA379993552E743(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xA751787977EEB3902E30E1D19CA00C6AD274A1F622C31A206E32366700B05674(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256 indexed arg0);

    mapping(uint256 => uint256) public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    mapping(uint256 => uint256) public slot_12;
    mapping(uint256 => uint256) public slot_14;
    mapping(uint256 => uint256) public slot_15;
    uint256 public slot_16;
    uint256 public slot_17;
    uint256 public slot_18;
    uint256 public slot_19;
    uint256 public slot_2;
    uint256 public slot_20;
    uint256 public slot_21;
    uint256 public slot_22;
    uint256 public slot_23;
    uint256 public slot_24;
    uint256 public slot_25;
    uint256 public slot_26;
    uint256 public slot_27;
    uint256 public slot_28;
    uint256 public slot_29;
    uint256 public slot_3;
    uint256 public slot_30;
    uint256 public slot_31;
    uint256 public slot_32;
    uint256 public slot_33;
    uint256 public slot_34;
    uint256 public slot_35;
    mapping(uint256 => uint256) public slot_36;
    mapping(uint256 => uint256) public slot_37;
    mapping(uint256 => uint256) public slot_38;
    mapping(uint256 => uint256) public slot_39;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x06f26aeb() public {
        // block_0:
    }

    function public_0x175a9f49() public {
        // block_0:
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5)) {
          // goto block_4
        }
        // goto block_1
        // block_1:
        if (!false /* TODO: evm.eq13 */) {
          // block_2:
        }
        // block_3:
        // block_4:
        revert(); // error_string, selector=0x8c379a0
    }

    function public_0x20c30c78() public {
        // block_0:
    }

    function public_0x2abdb49d() public returns (bool ret0) {
        // block_0:
        return slot_38[0 /* TODO: unresolved value: private__0x2d7c_0x2d7c */] != 0;
    }

    function public_0x5686d018() public {
        // block_0:
    }

    function public_0x5a186122() public {
        // block_0:
    }

    function public_0x5f0a549a() public returns (uint256 ret0) {
        // block_0:
        return slot_21;
    }

    function public_0x65b1ad11() public {
        // block_0:
    }

    function public_0x748b7feb() public {
        // block_0:
    }

    function public_0x779f6350() public {
        // block_0:
    }

    function public_0x8d8e0b61() public {
        // block_0:
    }

    function public_0xb07280cf() public returns (bool ret0) {
        // block_0:
        return slot_16 / uint256(256) & uint256(255) != 0;
    }

    function public_0xb533d443() public returns (uint256 ret0) {
        // block_0:
        return slot_18;
    }

    function public_0xc0193cc5() public returns (uint256 ret0) {
        // block_0:
        return slot_34;
    }

    function public_0xcd82c891() public returns (bool ret0) {
        // block_0:
        return slot_26 != 0;
    }

    function public_0xda739840() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function public_0xef982c24() public returns (uint256 ret0) {
        // block_0:
        return slot_17;
    }

    function isExcludedMaxTransactionAmount(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_37[uint256(uint160(arg0))] != 0;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0xdc5_0xdc5(arg1, uint256(uint160(arg0)), 503941) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_39[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_0[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function blacklistAccount(address arg0, bool arg1) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function buyDevFee() public returns (uint256 ret0) {
        // block_0:
        return slot_20;
    }

    function buyLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_19;
    }

    function buyMarketingFee() public returns (uint256 ret0) {
        // block_0:
        return slot_18;
    }

    function buyTotalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_17;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x1643_0x1643(arg1, uint256(uint160(arg0)), 505999) != 0;
    }

    function devWallet() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function disableTransferDelay() public returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_16 = slot_16 & uint256((type(uint256).max - 255));
          return 1;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function earlySellLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_29;
    }

    function earlySellMarketingFee() public returns (uint256 ret0) {
        // block_0:
        return slot_30;
    }

    function enableEarlySellTax() public returns (bool ret0) {
        // block_0:
        return slot_11 / uint256(16777216) & uint256(255) != 0;
    }

    function enableTrading() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_11 = slot_11 & uint256((type(uint256).max - 16776960)) | uint256(65792);
          slot_35 = block.number;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromFees(address arg0, bool arg1) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x108f_0x108f(arg1, uint256(uint160(arg0)), 504644) != 0;
    }

    function isExcludedFromFees(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_36[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
    }

    function limitsInEffect() public returns (bool ret0) {
        // block_0:
        return slot_11 != 0;
    }

    function marketingWallet() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function maxTransactionAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_8;
    }

    function maxWallet() public returns (uint256 ret0) {
        // block_0:
        return slot_10;
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimits() public returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_11 = slot_11 & uint256((type(uint256).max - 255));
          return 1;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function renounceOwnership() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5, 0); // TODO: recover event signature
          slot_5 = slot_5 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function sellDevFee() public returns (uint256 ret0) {
        // block_0:
        return slot_25;
    }

    function sellLiquidityFee() public returns (uint256 ret0) {
        // block_0:
        return slot_24;
    }

    function sellMarketingFee() public returns (uint256 ret0) {
        // block_0:
        return slot_23;
    }

    function sellTotalFees() public returns (uint256 ret0) {
        // block_0:
        return slot_22;
    }

    function setAutomatedMarketMakerPair(address arg0, bool arg1) public {
        // block_0:
    }

    function setEarlySellTax(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function swapEnabled() public returns (bool ret0) {
        // block_0:
        return slot_11 / uint256(65536) & uint256(255) != 0;
    }

    function swapTokensAtAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function symbol() public {
        // block_0:
    }

    function tokensForDev() public returns (uint256 ret0) {
        // block_0:
        return slot_33;
    }

    function tokensForLiquidity() public returns (uint256 ret0) {
        // block_0:
        return slot_32;
    }

    function tokensForMarketing() public returns (uint256 ret0) {
        // block_0:
        return slot_31;
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_2;
    }

    function tradingActive() public returns (bool ret0) {
        // block_0:
        return slot_11 / uint256(256) & uint256(255) != 0;
    }

    function transferDelayEnabled() public returns (bool ret0) {
        // block_0:
        return slot_16 != 0;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x2d3e_0x2d3e(uint256(uint160(arg0)), 11931);
        private__0x2d3e_0x2d3e(uint256(uint160(arg1)), 11947);
        return 1;
    }

    function transferOwnership(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_5 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_5 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x1690_0x1690(arg1, uint256(uint160(arg0)), 506093) != 0;
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(609523799656461207055263831161151521506352163005);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(697323163401596485410334513241460920685086001293);
    }

    function updateBuyFees(uint256 arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x13ac_0x13ac(arg2, arg1, arg0, 505467);
    }

    function updateDevWallet(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0x90B8024C4923D3873FF5B9FCB43D0360D4B9217FA41225D07BA379993552E743(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7); // TODO: recover event signature
          slot_7 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_7 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMarketingWallet(address arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0xA751787977EEB3902E30E1D19CA00C6AD274A1F622C31A206E32366700B05674(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6); // TODO: recover event signature
          slot_6 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_6 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMaxTxnAmount(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x301e_0x301e(private__0x301e_0x301e(private__0x3007_0x3007(5, slot_2, 3847), 1000, 3857), 1000000000000000000, 3867)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=47
          }
          else {
            // block_2:
            slot_8 = private__0x3007_0x3007(1000000000000000000, arg0, 3988);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMaxWalletAmount(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x301e_0x301e(private__0x301e_0x301e(private__0x3007_0x3007(15, slot_2, 6132), 1000, 6142), 1000000000000000000, 6152)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=36
          }
          else {
            // block_2:
            slot_10 = private__0x3007_0x3007(1000000000000000000, arg0, 6261);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSellFees(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) public {
        // block_0:
        require(msg.data.length > 163);
        // block_1:
        private__0x1b26_0x1b26(arg4, arg3, arg2, arg1, arg0, 507084);
    }

    function updateSwapEnabled(bool arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSwapTokensAtAmount(uint256 arg0) public returns (bool ret0) {
        // block_0:
        return private__0x187b_0x187b(arg0, 506609) != 0;
    }

    function private__0x1495_0x1495(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x108f_0x108f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1bda_0x1bda(private__0x26ab_0x26ab(arg0, slot_1[uint256(uint160(msg.sender))], 779121), arg1, uint256(uint160(msg.sender)), 643370);
        return 1;
    }

    function private__0x13ac_0x13ac(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_18 = arg2;
          slot_19 = arg1;
          slot_20 = arg0;
          temp_0 = private__0x313e_0x313e(private__0x313e_0x313e(arg2, arg1, 5104), arg0, 5114);
          slot_17 = temp_0;
          if (temp_0 < 51) {
            // block_2:
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=29
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1520_0x1520(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1 == (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(609523799656461207055263831161151521506352163005)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=57
          }
          else {
            // block_2:
            emit Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg0 != 0); // TODO: recover event signature
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1643_0x1643(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x1690_0x1690(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x187b_0x187b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x301e_0x301e(private__0x3007_0x3007(1, slot_2, 6335), 100000, 6345)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=53
          }
          else {
            // block_2:
            if (arg0 > private__0x301e_0x301e(private__0x3007_0x3007(5, slot_2, 6477), 1000, 6487)) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=52
            }
            else {
              // block_3:
              slot_9 = arg0;
              return 1;
            }
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1b26_0x1b26(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_23 = arg4;
          slot_24 = arg3;
          slot_25 = arg2;
          slot_29 = arg1;
          slot_30 = arg0;
          temp_0 = private__0x313e_0x313e(private__0x313e_0x313e(arg4, arg3, 7028), arg2, 7038);
          slot_22 = temp_0;
          if (temp_0 < 100) {
            // block_2:
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=29
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1bda_0x1bda(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x2673_0x2673(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 > arg2) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x31d9_0x31d9(arg2, arg1, 1052235);
        }
    }

    function private__0x26ab_0x26ab(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x313e_0x313e(arg1, arg0, 9911);
        if (temp_0 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=27
        }
        else {
          // block_1:
          return temp_0;
        }
    }

    function private__0x2763_0x2763(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
          }
        }
    }

    function private__0x29fa_0x29fa(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        if (arg1 == 0) {
          // block_2:
          return 0;
        }
        else {
          // block_1:
          temp_0 = private__0x3007_0x3007(arg1, arg0, 10772);
          if (private__0x301e_0x301e(temp_0, arg1, 10785) == arg0) {
            // block_3:
            return temp_0;
          }
          else {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=33
          }
        }
    }

    function private__0x2cb0_0x2cb0(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x301e_0x301e(arg2, arg1, 1052704);
        }
    }

    function private__0x2d3e_0x2d3e(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x2f86_0x2f86(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x3007_0x3007(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x301e_0x301e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x313e_0x313e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x3151_0x3151(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x3196_0x3196(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x31d9_0x31d9(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0xdc5_0xdc5(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1bda_0x1bda(arg0, arg1, uint256(uint160(msg.sender)), 507348);
        return 1;
    }

    function private__0xf9a_0xf9a(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }
}
