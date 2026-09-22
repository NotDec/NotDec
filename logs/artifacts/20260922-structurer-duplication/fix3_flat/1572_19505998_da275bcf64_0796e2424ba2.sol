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
    mapping(uint256 => uint256) public slot_35;
    mapping(uint256 => uint256) public slot_36;
    mapping(uint256 => uint256) public slot_37;
    mapping(uint256 => uint256) public slot_38;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x175a9f49() public {
        // block_0:
        if (msg.data.length > 35) {
          // goto block_1
        }
        // goto block_3
        // block_1:
        if (!false /* TODO: evm.eq.i.i */) {
          // goto block_2
        }
        // goto block_4
        // block_2:
        revert(); // empty
        // block_3:
        revert(); // empty
        // block_4:
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5)) {
          // goto block_9
        }
        // goto block_5
        // block_5:
        if (false /* TODO: evm.eq13 */) {
          // block_6:
        }
        else {
          // block_7:
        }
        // block_8:
        // block_9:
        revert(); // error_string, selector=0x8c379a0
    }

    function public_0x20c30c78() public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.gt.i */);
        // block_2:
        require(false /* TODO: evm.slt7.i */);
        // block_3:
        require(!false /* TODO: evm.gt28.i.not */);
        // block_8:
    }

    function public_0x2abdb49d() public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return slot_37[0 /* TODO: unresolved value: evm.calldataload.i.load */] != 0;
    }

    function public_0x5686d018() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
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
        require(msg.data.length > 35);
        // block_2:
    }

    function public_0x748b7feb() public {
        // block_0:
    }

    function public_0x779f6350() public {
        // block_0:
    }

    function public_0x8d8e0b61() public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.gt.i */);
        // block_2:
        require(false /* TODO: evm.slt7.i */);
        // block_3:
        require(!false /* TODO: evm.gt28.i.not */);
        // block_8:
    }

    function public_0xb07280cf() public returns (bool ret0) {
        // block_0:
        return slot_16 / uint256(256) & uint256(255) != 0;
    }

    function public_0xb533d443() public returns (uint256 ret0) {
        // block_0:
        return slot_18;
    }

    function public_0xcd82c891() public returns (bool ret0) {
        // block_0:
        return slot_26 != 0;
    }

    function public_0xda739840() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
        }
        else {
          // block_4:
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
        require(msg.data.length > 35);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 1024386);
        return slot_36[uint256(uint160(arg0))] != 0;
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11950);
        private__0x2ca1_0x2ca1(uint256(uint160(arg1)), 11966);
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11473);
        return private__0xd35_0xd35(arg1, uint256(uint160(arg0)), 497716) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 1024386);
        return slot_38[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 1024386);
        return slot_0[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function blacklistAccount(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11858);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
        }
        else {
          // block_6:
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
        require(msg.data.length > 67);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11473);
        return private__0x15bd_0x15bd(arg1, uint256(uint160(arg0)), 499774) != 0;
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
          slot_34 = block.number;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromFees(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11858);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
          emit Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11858);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11473);
        return private__0x1008_0x1008(arg1, uint256(uint160(arg0)), 498419) != 0;
    }

    function isExcludedFromFees(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 1024386);
        return slot_35[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] != 0;
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
        require(msg.data.length > 67);
        // block_1:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11858);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
    }

    function setEarlySellTax(bool arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
        }
        else {
          // block_6:
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
        uint256 temp_0;
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_80
        // block_1:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11797);
        private__0x2ca1_0x2ca1(uint256(uint160(arg1)), 11813);
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_78
        // block_2:
        if (uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_77
        }
        // goto block_3
        // block_8:
        if (!((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_9
        }
        // goto block_32
        // block_9:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_10
        }
        // goto block_32
        // block_10:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005)) {
          // goto block_11
        }
        // goto block_32
        // block_11:
        if (slot_5 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
          // goto block_12
        }
        // goto block_32
        // block_12:
        if (slot_11 / uint256(256) & uint256(255) == 0) {
          // goto block_13
        }
        // goto block_15
        // block_13:
        if (slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_14
        }
        // goto block_15
        // block_14:
        if (slot_35[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_74
        }
        // goto block_15
        // block_23:
        if (slot_38[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_25
        }
        // block_24:
        if (!(slot_36[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_25
        }
        // goto block_28
        // block_25:
        if (slot_36[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_26
        }
        // goto block_32
        // block_26:
        if (private__0x30b8_0x30b8(arg2, slot_0[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 8682) > slot_10) {
          // goto block_27
        }
        // goto block_32
        // block_27:
        require(false, "Max wallet exceeded");
        // block_28:
        if (arg2 > slot_8) {
          revert(); // error_string, selector=0x8c379a0, string_length=54
        }
        // block_32:
        if (!(block.number > private__0x30b8_0x30b8(0, slot_34, 8763))) {
          // block_33:
          if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(800380388807095800137317041925595855535296818508))) {
            // block_34:
            if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 697323163401596485410334513241460920685086001293)) {
              // block_35:
            }
          }
        }
        // block_36:
        if (slot_0[uint256(uint160(address(this)))] < slot_9) {
          // goto block_59
        }
        // block_37:
        if (slot_11 / uint256(65536) & uint256(255) == 0) {
          // goto block_59
        }
        // block_38:
        if (!(slot_5 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_59
        }
        // block_39:
        if (!(slot_38[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_59
        }
        // block_40:
        if (!(slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_59
        }
        // block_41:
        if (!(slot_35[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_59
        }
        // block_42:
        slot_5 = uint256(1) << uint256(160) | slot_5 & ~(uint256(255) << uint256(160));
        temp_0 = private__0x30b8_0x30b8(private__0x30b8_0x30b8(slot_32, slot_31, 10248), slot_33, 10258);
        if (false /* TODO: spec.select */) {
          // goto block_58
        }
        // block_43:
        if (slot_0[uint256(uint160(address(this)))] > private__0x2f81_0x2f81(20, slot_9, 10296)) {
          // block_44:
        }
        // block_45:
        if (!false /* TODO: evm.iszero.i.i.i.not */) {
          // goto block_46
        }
        // goto block_53
        // block_46:
        while (false /* TODO: evm.lt50.i.i.i.not */) {
          // block_52:
        }
        // block_48:
        if (!false /* TODO: evm.iszero62.i.i.i.not */) {
          // goto block_49
        }
        // goto block_51
        // block_49:
        if (false /* TODO: evm.iszero68.i.i.i.not */) {
          // goto block_50
        }
        // goto block_54
        // block_50:
        revert(); // returndata_forward
        // block_51:
        revert(); // empty
        // block_53:
        revert(); // returndata_forward
        // block_54:
        slot_32 = 0;
        slot_31 = 0;
        slot_33 = 0;
        if (!false /* TODO: evm.eq.i.i */) {
          // block_55:
        }
        // block_56:
        if (!false /* TODO: evm.eq47.i.i */) {
          // goto block_57
        }
        // goto block_58
        // block_57:
        // block_58:
        slot_5 = slot_5 & ~(uint256(255) << uint256(160));
        // block_59:
        if (slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_60
        }
        // goto block_81
        // block_60:
        if (!false /* TODO: cond */) {
          // goto block_61
        }
        // goto block_82
        // block_61:
        if (slot_38[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_63
        }
        // block_62:
        if (slot_22 == 0) {
          // goto block_63
        }
        // goto block_66
        // block_63:
        if (!(slot_38[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_64:
          if (!(slot_17 == 0)) {
            // block_65:
          }
        }
        // goto block_67
        // block_66:
        // block_67:
        if (!false /* TODO: evm.iszero441.i */) {
          // block_68:
        }
        // block_69:
        // block_70:
        // goto block_79
        // block_15:
        if (!(slot_16 == 0)) {
          // block_16:
          if (!((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_17:
            if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) == (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(697323163401596485410334513241460920685086001293))) {
              // block_18:
              if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg1)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(800380388807095800137317041925595855535296818508))) {
                // block_19:
                if (!(block.number > slot_12[uint256(uint160(tx.origin))])) {
                  // block_73:
                  revert(); // error_string, selector=0x8c379a0, string_length=73
                }
                // block_20:
              }
            }
          }
        }
        // block_21:
        if (slot_38[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_23
        }
        // block_22:
        if (!(slot_36[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_23
        }
        // goto block_30
        // block_30:
        if (arg2 > slot_8) {
          // block_72:
          revert(); // error_string, selector=0x8c379a0, string_length=53
        }
        // block_31:
        // block_71:
        require(false, "Max wallet exceeded");
        // block_74:
        require(false, "Trading is not active.");
        // block_3:
        if (!(slot_14[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_76:
          revert(); // error_string, selector=0x8c379a0, string_length=49
        }
        // block_4:
        if (!(slot_14[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_76
        }
        // goto block_5
        // block_5:
        // block_75:
        private__0x26dd_0x26dd(0, uint256(uint160(arg1)), uint256(uint160(arg0)), 1023693);
        // goto block_79
        // block_77:
        revert(); // error_string, selector=0x8c379a0
        // block_78:
        revert(); // error_string, selector=0x8c379a0
        // block_82:
        // block_79:
        return 1;
        // block_80:
        revert(); // empty
        // block_81:
        // goto block_79
        // TODO: recover remaining body
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 1024386);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_4:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_5 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_5 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        require(msg.data.length > 67);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 11473);
        if (!(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_79
        // block_3:
        if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // goto block_78
        }
        // goto block_4
        // block_10:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_11
        }
        // goto block_33
        // block_11:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005)) {
          // goto block_12
        }
        // goto block_33
        // block_12:
        if (slot_5 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
          // goto block_13
        }
        // goto block_33
        // block_13:
        if (slot_11 / uint256(256) & uint256(255) == 0) {
          // goto block_14
        }
        // goto block_16
        // block_14:
        if (slot_35[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_15
        }
        // goto block_16
        // block_15:
        if (slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_75
        }
        // goto block_16
        // block_24:
        if (slot_38[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_26
        }
        // block_25:
        if (!(slot_36[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_26
        }
        // goto block_29
        // block_26:
        if (slot_36[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_27
        }
        // goto block_33
        // block_27:
        if (private__0x30b8_0x30b8(arg1, slot_0[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 8682) > slot_10) {
          // goto block_28
        }
        // goto block_33
        // block_28:
        require(false, "Max wallet exceeded");
        // block_29:
        if (arg1 > slot_8) {
          revert(); // error_string, selector=0x8c379a0, string_length=54
        }
        // block_33:
        if (!(block.number > private__0x30b8_0x30b8(0, slot_34, 8763))) {
          // block_34:
          if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(800380388807095800137317041925595855535296818508))) {
            // block_35:
            if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 697323163401596485410334513241460920685086001293)) {
              // block_36:
            }
          }
        }
        // block_37:
        if (slot_0[uint256(uint160(address(this)))] < slot_9) {
          // goto block_60
        }
        // block_38:
        if (slot_11 / uint256(65536) & uint256(255) == 0) {
          // goto block_60
        }
        // block_39:
        if (!(slot_5 / (uint256(1) << uint256(160)) & uint256(255) == 0)) {
          // goto block_60
        }
        // block_40:
        if (!(slot_38[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_60
        }
        // block_41:
        if (!(slot_35[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_60
        }
        // block_42:
        if (!(slot_35[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_60
        }
        // block_43:
        slot_5 = uint256(1) << uint256(160) | slot_5 & ~(uint256(255) << uint256(160));
        temp_0 = private__0x30b8_0x30b8(private__0x30b8_0x30b8(slot_32, slot_31, 10248), slot_33, 10258);
        if (false /* TODO: spec.select */) {
          // goto block_59
        }
        // block_44:
        if (slot_0[uint256(uint160(address(this)))] > private__0x2f81_0x2f81(20, slot_9, 10296)) {
          // block_45:
        }
        // block_46:
        if (false /* TODO: evm.iszero.i.i.i.i.not */) {
          // goto block_54
        }
        while (true) {
          // block_47:
          // block_48:
          if (!false /* TODO: evm.lt50.i.i.i.i.not */) {
            break;
          }
          continue;
        }
        // block_49:
        if (!false /* TODO: evm.iszero62.i.i.i.i.not */) {
          // goto block_50
        }
        // goto block_52
        // block_50:
        if (false /* TODO: evm.iszero68.i.i.i.i.not */) {
          // goto block_51
        }
        // goto block_55
        // block_51:
        revert(); // returndata_forward
        // block_52:
        revert(); // empty
        // block_54:
        revert(); // returndata_forward
        // block_55:
        slot_32 = 0;
        slot_31 = 0;
        slot_33 = 0;
        if (!false /* TODO: evm.eq.i.i.i */) {
          // block_56:
        }
        // block_57:
        if (!false /* TODO: evm.eq47.i.i.i */) {
          // goto block_58
        }
        // goto block_59
        // block_58:
        // block_59:
        slot_5 = slot_5 & ~(uint256(255) << uint256(160));
        // block_60:
        if (slot_35[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_61
        }
        // goto block_81
        // block_61:
        if (!false /* TODO: cond */) {
          // goto block_62
        }
        // goto block_82
        // block_62:
        if (!(slot_38[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_63
        }
        // goto block_64
        // block_63:
        if (slot_22 == 0) {
          // goto block_64
        }
        // goto block_66
        // block_64:
        if (!(slot_38[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_65:
          if (!(slot_17 == 0)) {
            // block_66:
          }
        }
        // block_68:
        if (!false /* TODO: evm.iszero441.i.i */) {
          // block_69:
        }
        // block_70:
        // block_71:
        // goto block_80
        // block_16:
        if (!(slot_16 == 0)) {
          // block_17:
          if (!((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_18:
            if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(697323163401596485410334513241460920685086001293))) {
              // block_19:
              if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(800380388807095800137317041925595855535296818508))) {
                // block_20:
                if (!(block.number > slot_12[uint256(uint160(tx.origin))])) {
                  // block_74:
                  revert(); // error_string, selector=0x8c379a0, string_length=73
                }
                // block_21:
              }
            }
          }
        }
        // block_22:
        if (slot_38[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_24
        }
        // block_23:
        if (!(slot_36[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_24
        }
        // goto block_31
        // block_31:
        if (arg1 > slot_8) {
          // block_73:
          revert(); // error_string, selector=0x8c379a0, string_length=53
        }
        // block_32:
        // block_72:
        require(false, "Max wallet exceeded");
        // block_75:
        require(false, "Trading is not active.");
        // block_4:
        if (!(slot_14[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_77:
          revert(); // error_string, selector=0x8c379a0, string_length=49
        }
        // block_5:
        if (!(slot_14[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_77
        }
        // goto block_6
        // block_6:
        // block_76:
        private__0x26dd_0x26dd(0, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 1023693);
        // goto block_80
        // block_78:
        revert(); // error_string, selector=0x8c379a0
        // block_79:
        revert(); // error_string, selector=0x8c379a0
        // block_82:
        // block_80:
        return 1;
        // block_81:
        // goto block_80
        // TODO: recover remaining body
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(800380388807095800137317041925595855535296818508);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(697323163401596485410334513241460920685086001293);
    }

    function updateBuyFees(uint256 arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x1326_0x1326(arg2, arg1, arg0, 499242);
    }

    function updateDevWallet(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 1024386);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          emit Event_0x90B8024C4923D3873FF5B9FCB43D0360D4B9217FA41225D07BA379993552E743(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7); // TODO: recover event signature
          slot_7 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_7 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMarketingWallet(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x2ca1_0x2ca1(uint256(uint160(arg0)), 1024386);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          emit Event_0xA751787977EEB3902E30E1D19CA00C6AD274A1F622C31A206E32366700B05674(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6); // TODO: recover event signature
          slot_6 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_6 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMaxTxnAmount(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (arg0 < private__0x2f98_0x2f98(private__0x2f98_0x2f98(private__0x2f81_0x2f81(5, slot_2, 3712), 1000, 3722), 1000000000000000000, 3732)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=47
          }
          else {
            // block_4:
            slot_8 = private__0x2f81_0x2f81(1000000000000000000, arg0, 3853);
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateMaxWalletAmount(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          if (arg0 < private__0x2f98_0x2f98(private__0x2f98_0x2f98(private__0x2f81_0x2f81(15, slot_2, 5998), 1000, 6008), 1000000000000000000, 6018)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=36
          }
          else {
            // block_4:
            slot_10 = private__0x2f81_0x2f81(1000000000000000000, arg0, 6127);
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSellFees(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) public {
        // block_0:
        require(msg.data.length > 163);
        // block_1:
        private__0x1aa0_0x1aa0(arg4, arg3, arg2, arg1, arg0, 500814);
    }

    function updateSwapEnabled(bool arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_5:
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function updateSwapTokensAtAmount(uint256 arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return private__0x17f5_0x17f5(arg0, 500339) != 0;
    }

    function private__0x140f_0x140f(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1008_0x1008(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1b54_0x1b54(private__0x2625_0x2625(arg0, slot_1[uint256(uint160(msg.sender))], 761753), arg1, uint256(uint160(msg.sender)), 631514);
        return 1;
    }

    function private__0x1326_0x1326(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_18 = arg2;
          slot_19 = arg1;
          slot_20 = arg0;
          temp_0 = private__0x30b8_0x30b8(private__0x30b8_0x30b8(arg2, arg1, 4970), arg0, 4980);
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

    function private__0x149a_0x149a(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1 == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(800380388807095800137317041925595855535296818508)) {
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

    function private__0x15bd_0x15bd(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x17f5_0x17f5(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 < private__0x2f98_0x2f98(private__0x2f81_0x2f81(1, slot_2, 6201), 100000, 6211)) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=53
          }
          else {
            // block_2:
            if (arg0 > private__0x2f98_0x2f98(private__0x2f81_0x2f81(5, slot_2, 6343), 1000, 6353)) {
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

    function private__0x1aa0_0x1aa0(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_23 = arg4;
          slot_24 = arg3;
          slot_25 = arg2;
          slot_29 = arg1;
          slot_30 = arg0;
          temp_0 = private__0x30b8_0x30b8(private__0x30b8_0x30b8(arg4, arg3, 6894), arg2, 6904);
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

    function private__0x1b54_0x1b54(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x25ed_0x25ed(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 > arg2) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x3153_0x3153(arg2, arg1, 1023843);
        }
    }

    function private__0x2625_0x2625(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x30b8_0x30b8(arg1, arg0, 9777);
        if (temp_0 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=27
        }
        else {
          // block_1:
          return temp_0;
        }
    }

    function private__0x26dd_0x26dd(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x2974_0x2974(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        if (arg1 == 0) {
          // block_2:
          return 0;
        }
        else {
          // block_1:
          temp_0 = private__0x2f81_0x2f81(arg1, arg0, 10638);
          if (private__0x2f98_0x2f98(temp_0, arg1, 10651) == arg0) {
            // block_3:
            return temp_0;
          }
          else {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=33
          }
        }
    }

    function private__0x2c2a_0x2c2a(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          return private__0x2f98_0x2f98(arg2, arg1, 1024312);
        }
    }

    function private__0x2ca1_0x2ca1(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x2f38_0x2f38(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x2f81_0x2f81(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x2f98_0x2f98(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x30b8_0x30b8(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x30cb_0x30cb(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x3110_0x3110(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x3153_0x3153(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0xd35_0xd35(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1b54_0x1b54(arg0, arg1, uint256(uint160(msg.sender)), 501004);
        return 1;
    }

    function private__0xf13_0xf13(uint256 arg0, uint256 arg1) internal {
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
