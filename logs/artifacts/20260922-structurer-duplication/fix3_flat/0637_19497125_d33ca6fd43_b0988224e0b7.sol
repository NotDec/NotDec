contract Decompiled {
    event Event_0xCB5F36DF892836A2EAEDC349DE29A7581176990398EE185D16EAA8F6C1ABD8F1();
    event Event_0x38513C502B0AB4834AC1DF9502B76F75DCF7092469782CFD0DB7FE664388E25E();
    event Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(uint256 indexed arg0);
    event Event_0x12528A3C61E0F3B2D6FC707A9FC58B1AF86E252CAD0D7F4C154EBEABB162DACE();
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(uint256 indexed arg0, bool indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xC246820312F1BE47E3958D661D0C150C01B96D1FE3DF1E38EDD76693FFA8122B(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xB3DA2DB3DFC3778F99852546C6E9AB39EC253F9DE7B0847AFEC61BD27878E923(uint256 indexed arg0);
    event Event_0xF4EAA75EAE08AE80C3DAF791438DAC1CFF2CFD3B0BAD2304EC7BBB067E502616(uint256 indexed arg0);
    event Event_0x9D786082B596FD0E7A9B16EEB0AEDC913596A0DA2D30B821F8D7E3556E80C917(uint256 indexed arg0);
    event Event_0xA90D7598849FFFFB63CB2A2B23157FA85704D50370070DE404A308243FE1DAEC(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x52CD2CDB42FF0EEEC9362D7ED5B04F64C8D022697128B5378FC51CEA7E63C779();
    event Event_0x26E776FCF7CA20AA79B5B946E9B5111F47205539ECE9D7A7995271DD6A8B5BAD(uint256 indexed arg0);
    event Event_0xFF3DD5E80294197918C284BBFC3DADD97D0B40CE92106110946329088F80068A();
    event Event_0x4B89C347592B1D537E066CB4ED98D87696AE35164745D7E370E4ADD16941DC92(uint256 indexed arg0);

    mapping(uint256 => uint256) public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    uint256 public slot_12;
    mapping(uint256 => uint256) public slot_13;
    mapping(uint256 => uint256) public slot_14;
    uint256 public slot_15;
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
    mapping(uint256 => uint256) public slot_25;
    mapping(uint256 => uint256) public slot_26;
    mapping(uint256 => uint256) public slot_27;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x14d178ac() public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(false /* TODO: evm.gt.i */);
        // block_2:
        require(false /* TODO: evm.slt7.i */);
        // block_3:
        require(false /* TODO: evm.gt11.i */);
        // block_4:
        require(!false /* TODO: evm.gt18.i.not */);
        // block_5:
        require(false /* TODO: evm.eq.i.i */);
        // block_12:
        // block_15:
        revert(); // error_string, selector=0x8c379a0
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5)) {
          // goto block_15
        }
        while (false /* TODO: evm.lt.i.not */) {
          // block_14:
        }
        // block_16:
        // TODO: recover remaining body
    }

    function public_0x515fc002() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_7 = uint256(1) << uint256(168) | slot_7 & ~(uint256(255) << uint256(168));
          emit Event_0xB3DA2DB3DFC3778F99852546C6E9AB39EC253F9DE7B0847AFEC61BD27878E923(block.timestamp); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x56224ea7() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          require(slot_10 / uint256(256) & uint256(255) != 0, "wl disabled");
          // block_2:
          slot_10 = slot_10 & uint256((type(uint256).max - 65280));
          slot_18 = 30;
          slot_19 = 0;
          slot_17 = private__0x2a33_0x2a33(30, 0, 2916);
          slot_21 = 40;
          slot_22 = 0;
          slot_20 = private__0x2a33_0x2a33(40, 0, 2941);
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x598ae6e4() public returns (uint256 ret0, uint256 ret1, uint256 ret2) {
        // block_0:
        return (slot_7 / (uint256(1) << uint256(168)) & uint256(255) != 0 ? 1 : 0, slot_8, slot_9);
    }

    function public_0x683c3b39() public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          slot_19 = 0;
          slot_22 = 0;
          emit Event_0x9D786082B596FD0E7A9B16EEB0AEDC913596A0DA2D30B821F8D7E3556E80C917(block.timestamp); // TODO: recover event signature
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x6f013db9() public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3) {
        // block_0:
        return (slot_10 / uint256(65536) & uint256(255) != 0 ? 1 : 0, slot_10 / uint256(16777216) & uint256(255) != 0 ? 1 : 0, slot_12, slot_11);
    }

    function public_0x75cd6818() public {
        // block_0:
    }

    function public_0x924f364d() public returns (uint256 ret0, uint256 ret1, uint256 ret2) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        return (slot_27[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: evm.calldataload.i.load */] != 0 ? 1 : 0, 0 /* TODO: unresolved value: evm_sload */ & uint256(255) != 0 ? 1 : 0, 0 /* TODO: unresolved value: evm_sload */ & uint256(255) != 0 ? 1 : 0);
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10627);
        private__0x27a5_0x27a5(uint256(uint160(arg1)), 10643);
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10197);
        return private__0x837_0x837(arg1, uint256(uint160(arg0)), 315444) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 508694);
        return slot_0[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10197);
        if (slot_1[uint256(uint160(msg.sender))] < arg1) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_3:
          private__0x18ab_0x18ab(slot_1[uint256(uint160(msg.sender))] - arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 5035);
          return 1;
        }
    }

    function disableTransferDelay() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_10 = slot_10 & uint256((type(uint256).max - 4278190080));
          emit Event_0x26E776FCF7CA20AA79B5B946E9B5111F47205539ECE9D7A7995271DD6A8B5BAD(block.timestamp); // TODO: recover event signature
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
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10528);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
    }

    function excludeFromMaxTransaction(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10528);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
    }

    function getFeeInfo() public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3, uint256 ret4, uint256 ret5) {
        // block_0:
    }

    function getFeeWallet() public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        return ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_16, slot_15 & (uint256(1) << uint256(160)) + uint256(type(uint256).max));
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10197);
        return private__0xa23_0xa23(arg1, uint256(uint160(arg0)), 315724) != 0;
    }

    function isLive() public returns (bool ret0) {
        // block_0:
        return slot_10 != 0;
    }

    function name() public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_7 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimit() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_18 = 2;
          slot_19 = 0;
          slot_17 = private__0x2a33_0x2a33(2, 0, 3240);
          slot_21 = 2;
          slot_22 = 0;
          slot_20 = private__0x2a33_0x2a33(2, 0, 3265);
          slot_10 = slot_10 & uint256((type(uint256).max - 16711680));
          emit Event_0xF4EAA75EAE08AE80C3DAF791438DAC1CFF2CFD3B0BAD2304EC7BBB067E502616(block.timestamp); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function renounceOwnership() public {
        // block_0:
        private__0xd84_0xd84(316101);
    }

    function setAutomatedMarketMakerPair(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10528);
        require(false /* TODO: evm.eq.i.i */);
        // block_4:
    }

    function setBuyFees(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x13c1_0x13c1(arg1, arg0, 316517);
    }

    function setDevWallet(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 508694);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          emit Event_0xC246820312F1BE47E3958D661D0C150C01B96D1FE3DF1E38EDD76693FFA8122B(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_15); // TODO: recover event signature
          slot_15 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_15 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setMarketing(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 508694);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_3:
          emit Event_0xA90D7598849FFFFB63CB2A2B23157FA85704D50370070DE404A308243FE1DAEC(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_16); // TODO: recover event signature
          slot_16 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_16 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setSellFees(uint256 arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        private__0x122a_0x122a(arg1, arg0, 316390);
    }

    function setSwapBackSettings(bool arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(false /* TODO: evm.eq.i */);
        // block_3:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_4:
          if (arg1 == 0) {
            // block_8:
            revert(); // error_string, selector=0x8c379a0, string_length=52
          }
          else {
            // block_5:
            if (arg2 < arg1) {
              // block_7:
              revert(); // error_string, selector=0x8c379a0, string_length=42
            }
            else {
              // block_6:
              slot_8 = private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(slot_2, arg1, 5713), 10000, 5723);
              slot_9 = private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(slot_2, arg2, 5749), 10000, 5759);
              emit Event_0x52CD2CDB42FF0EEEC9362D7ED5B04F64C8D022697128B5378FC51CEA7E63C779(); // TODO: recover event signature
            }
          }
        }
        else {
          // block_9:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setTxLimit(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0xb82_0xb82(arg0, 315917);
    }

    function setWalletLimit(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x172d_0x172d(arg0, 316741);
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_2;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_80
        // block_1:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10412);
        private__0x27a5_0x27a5(uint256(uint160(arg1)), 10428);
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_2
        }
        // goto block_76
        // block_2:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_75
        // block_3:
        if (!(arg2 == 0)) {
          // goto block_5
        }
        // block_4:
        if (!(slot_27[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_5
        }
        // goto block_74
        // block_5:
        if (!(slot_10 / uint256(65536) & uint256(255) == 0)) {
          // goto block_6
        }
        // goto block_34
        // block_6:
        if (!((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_7
        }
        // goto block_34
        // block_7:
        if (!((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_8
        }
        // goto block_34
        // block_8:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_9
        }
        // goto block_34
        // block_9:
        if (!(uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005)) {
          // goto block_10
        }
        // goto block_34
        // block_10:
        if (slot_7 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
          // goto block_11
        }
        // goto block_34
        // block_11:
        if (!(slot_10 == 0)) {
          // goto block_14
        }
        // block_12:
        if (!(slot_27[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_14
        }
        // block_13:
        if (!(slot_27[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_14
        }
        // goto block_73
        // block_14:
        if (!(slot_10 / uint256(256) & uint256(255) == 0)) {
          // goto block_15
        }
        // goto block_17
        // block_15:
        if (slot_14[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_16
        }
        // goto block_17
        // block_16:
        if (slot_14[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_72
        }
        // goto block_17
        // block_25:
        if (slot_25[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_27
        }
        // block_26:
        if (!(slot_26[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_27
        }
        // goto block_30
        // block_27:
        if (slot_26[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_28
        }
        // goto block_34
        // block_28:
        if (private__0x2a33_0x2a33(arg2, slot_0[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 8041) > slot_12) {
          // goto block_29
        }
        // goto block_34
        // block_29:
        require(false, "Max wallet exceeded");
        // block_30:
        if (arg2 > slot_11) {
          revert(); // error_string, selector=0x8c379a0, string_length=45
        }
        // block_34:
        if (!(slot_0[uint256(uint160(address(this)))] < slot_8)) {
          // block_35:
        }
        // block_36:
        if (!false /* TODO: _0x1fda_0x0.i.in.reload */) {
          // goto block_38
        }
        // block_37:
        // block_38:
        if (!false /* TODO: _0x1fef_0x0.i.reload */) {
          // goto block_39
        }
        // goto block_56
        // block_39:
        if (slot_25[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_40
        }
        // goto block_56
        // block_40:
        if (slot_27[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_41
        }
        // goto block_56
        // block_41:
        if (slot_27[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_42
        }
        // goto block_56
        // block_42:
        if (!(arg2 < slot_8)) {
          // goto block_43
        }
        // goto block_56
        // block_43:
        slot_7 = uint256(1) << uint256(160) | slot_7 & ~(uint256(255) << uint256(160));
        if (!(slot_0[uint256(uint160(address(this)))] == 0)) {
          // goto block_44
        }
        // goto block_55
        // block_44:
        if (false /* TODO: evm.iszero.i.i.i.not */) {
          // goto block_52
        }
        while (true) {
          // block_45:
          // block_46:
          if (!false /* TODO: evm.lt48.i.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_53
        // block_52:
        revert(); // returndata_forward
        // block_47:
        require(!false /* TODO: evm.iszero60.i.i.i.not */);
        // block_48:
        if (false /* TODO: evm.iszero66.i.i.i.not */) {
          // block_49:
          revert(); // returndata_forward
        }
        // block_53:
        if (!(address(this).balance == 0)) {
          // block_54:
        }
        // block_55:
        slot_7 = slot_7 & ~(uint256(255) << uint256(160));
        // block_56:
        if (slot_27[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_57:
        }
        // block_58:
        if (slot_7 / (uint256(1) << uint256(168)) & uint256(255) == 0) {
          // goto block_77
        }
        // block_59:
        if (false /* TODO: brmerge */) {
          // goto block_77
        }
        // block_60:
        if (slot_25[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_62
        }
        // block_61:
        if (slot_20 == 0) {
          // goto block_62
        }
        // goto block_65
        // block_62:
        if (!(slot_25[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_63:
          if (!(slot_17 == 0)) {
            // block_64:
            temp_0 = private__0x23c3_0x23c3(100, private__0x23b8_0x23b8(slot_17, arg2, 508322), 8752);
            slot_23 = private__0x2a33_0x2a33(slot_23, private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(temp_0, slot_19, 8771), slot_17, 8781), 8797);
            slot_24 = private__0x2a33_0x2a33(slot_24, private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(temp_0, slot_18, 8818), slot_17, 8828), 8844);
          }
        }
        // goto block_66
        // block_65:
        temp_1 = private__0x23c3_0x23c3(100, private__0x23b8_0x23b8(slot_20, arg2, 508285), 8578);
        slot_23 = private__0x2a33_0x2a33(slot_23, private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(temp_1, slot_22, 8597), slot_20, 8607), 8623);
        slot_24 = private__0x2a33_0x2a33(slot_24, private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(temp_1, slot_21, 8644), slot_20, 8654), 8670);
        // block_66:
        if (!false /* TODO: evm.iszero425.i */) {
          // block_67:
        }
        // block_68:
        // goto block_77
        // block_17:
        if (!(slot_10 / uint256(16777216) & uint256(255) == 0)) {
          // block_18:
          if (!((slot_5 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_19:
            if (!((slot_6 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
              // block_20:
              if (!((slot_7 ^ uint256(uint160(arg1))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
                // block_21:
                if (!(block.number > slot_13[uint256(uint160(tx.origin))])) {
                  // block_71:
                  revert(); // error_string, selector=0x8c379a0, string_length=73
                }
                // block_22:
              }
            }
          }
        }
        // block_23:
        if (slot_25[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_25
        }
        // block_24:
        if (!(slot_26[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_25
        }
        // goto block_32
        // block_32:
        if (arg2 > slot_11) {
          // block_70:
          revert(); // error_string, selector=0x8c379a0, string_length=44
        }
        // block_33:
        // block_69:
        require(false, "Max wallet exceeded");
        // block_72:
        revert(); // error_string, selector=0x8c379a0, string_length=36
        // block_73:
        revert(); // error_string, selector=0x8c379a0, string_length=34
        // block_74:
        revert(); // error_string, selector=0x8c379a0, string_length=34
        // block_75:
        revert(); // error_string, selector=0x8c379a0
        // block_76:
        revert(); // error_string, selector=0x8c379a0
        // block_77:
        if (arg2 + uint256(type(uint256).max) < slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)]) {
          // block_78:
          private__0x18ab_0x18ab(slot_1[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 2584);
          return 1;
        }
        else {
          // block_79:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        // block_80:
        revert(); // empty
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 508694);
        private__0x1810_0x1810(uint256(uint160(arg0)), 316774);
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        uint256 temp_1;
        require(msg.data.length > 67);
        // block_2:
        private__0x27a5_0x27a5(uint256(uint160(arg0)), 10197);
        if (!(uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_3
        }
        // goto block_77
        // block_3:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_4
        }
        // goto block_76
        // block_4:
        if (!(arg1 == 0)) {
          // goto block_6
        }
        // block_5:
        if (!(slot_27[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_6
        }
        // goto block_75
        // block_6:
        if (!(slot_10 / uint256(65536) & uint256(255) == 0)) {
          // goto block_7
        }
        // goto block_35
        // block_7:
        if (!((slot_5 ^ uint256(uint160(msg.sender))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_8
        }
        // goto block_35
        // block_8:
        if (!((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_9
        }
        // goto block_35
        // block_9:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
          // goto block_10
        }
        // goto block_35
        // block_10:
        if (!(uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 57005)) {
          // goto block_11
        }
        // goto block_35
        // block_11:
        if (slot_7 / (uint256(1) << uint256(160)) & uint256(255) == 0) {
          // goto block_12
        }
        // goto block_35
        // block_12:
        if (!(slot_10 == 0)) {
          // goto block_15
        }
        // block_13:
        if (!(slot_27[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_15
        }
        // block_14:
        if (!(slot_27[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_15
        }
        // goto block_74
        // block_15:
        if (!(slot_10 / uint256(256) & uint256(255) == 0)) {
          // goto block_16
        }
        // goto block_18
        // block_16:
        if (slot_14[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_17
        }
        // goto block_18
        // block_17:
        if (slot_14[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_73
        }
        // goto block_18
        // block_26:
        if (slot_25[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_28
        }
        // block_27:
        if (!(slot_26[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_28
        }
        // goto block_31
        // block_28:
        if (slot_26[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_29
        }
        // goto block_35
        // block_29:
        if (private__0x2a33_0x2a33(arg1, slot_0[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], 8041) > slot_12) {
          // goto block_30
        }
        // goto block_35
        // block_30:
        require(false, "Max wallet exceeded");
        // block_31:
        if (arg1 > slot_11) {
          revert(); // error_string, selector=0x8c379a0, string_length=45
        }
        // block_35:
        if (!(slot_0[uint256(uint160(address(this)))] < slot_8)) {
          // block_36:
        }
        // block_37:
        if (!false /* TODO: _0x1fda_0x0.i.i.in.reload */) {
          // goto block_39
        }
        // block_38:
        // block_39:
        if (!false /* TODO: _0x1fef_0x0.i.i.reload */) {
          // goto block_40
        }
        // goto block_57
        // block_40:
        if (slot_25[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_41
        }
        // goto block_57
        // block_41:
        if (slot_27[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_42
        }
        // goto block_57
        // block_42:
        if (slot_27[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_43
        }
        // goto block_57
        // block_43:
        if (!(arg1 < slot_8)) {
          // goto block_44
        }
        // goto block_57
        // block_44:
        slot_7 = uint256(1) << uint256(160) | slot_7 & ~(uint256(255) << uint256(160));
        if (!(slot_0[uint256(uint160(address(this)))] == 0)) {
          // goto block_45
        }
        // goto block_56
        // block_45:
        if (false /* TODO: evm.iszero.i.i.i.i.not */) {
          // goto block_53
        }
        while (true) {
          // block_46:
          // block_47:
          if (!false /* TODO: evm.lt48.i.i.i.i.not */) {
            break;
          }
          continue;
        }
        // goto block_54
        // block_53:
        revert(); // returndata_forward
        // block_48:
        require(!false /* TODO: evm.iszero60.i.i.i.i.not */);
        // block_49:
        if (false /* TODO: evm.iszero66.i.i.i.i.not */) {
          // block_50:
          revert(); // returndata_forward
        }
        // block_54:
        if (!(address(this).balance == 0)) {
          // block_55:
        }
        // block_56:
        slot_7 = slot_7 & ~(uint256(255) << uint256(160));
        // block_57:
        if (slot_27[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // block_58:
        }
        // block_59:
        if (slot_7 / (uint256(1) << uint256(168)) & uint256(255) == 0) {
          // goto block_78
        }
        // block_60:
        if (false /* TODO: brmerge */) {
          // goto block_78
        }
        // block_61:
        if (slot_25[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_63
        }
        // block_62:
        if (slot_20 == 0) {
          // goto block_63
        }
        // goto block_66
        // block_63:
        if (!(slot_25[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // block_64:
          if (!(slot_17 == 0)) {
            // block_65:
            temp_0 = private__0x23c3_0x23c3(100, private__0x23b8_0x23b8(slot_17, arg1, 508322), 8752);
            slot_23 = private__0x2a33_0x2a33(slot_23, private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(temp_0, slot_19, 8771), slot_17, 8781), 8797);
            slot_24 = private__0x2a33_0x2a33(slot_24, private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(temp_0, slot_18, 8818), slot_17, 8828), 8844);
          }
        }
        // goto block_67
        // block_66:
        temp_1 = private__0x23c3_0x23c3(100, private__0x23b8_0x23b8(slot_20, arg1, 508285), 8578);
        slot_23 = private__0x2a33_0x2a33(slot_23, private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(temp_1, slot_22, 8597), slot_20, 8607), 8623);
        slot_24 = private__0x2a33_0x2a33(slot_24, private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(temp_1, slot_21, 8644), slot_20, 8654), 8670);
        // block_67:
        if (!false /* TODO: evm.iszero425.i.i */) {
          // block_68:
        }
        // block_69:
        // goto block_78
        // block_18:
        if (!(slot_10 / uint256(16777216) & uint256(255) == 0)) {
          // block_19:
          if (!((slot_5 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
            // block_20:
            if (!((slot_6 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
              // block_21:
              if (!((slot_7 ^ uint256(uint160(arg0))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0)) {
                // block_22:
                if (!(block.number > slot_13[uint256(uint160(tx.origin))])) {
                  // block_72:
                  revert(); // error_string, selector=0x8c379a0, string_length=73
                }
                // block_23:
              }
            }
          }
        }
        // block_24:
        if (slot_25[uint256(uint160(msg.sender)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0) {
          // goto block_26
        }
        // block_25:
        if (!(slot_26[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == 0)) {
          // goto block_26
        }
        // goto block_33
        // block_33:
        if (arg1 > slot_11) {
          // block_71:
          revert(); // error_string, selector=0x8c379a0, string_length=44
        }
        // block_34:
        // block_70:
        require(false, "Max wallet exceeded");
        // block_73:
        revert(); // error_string, selector=0x8c379a0, string_length=36
        // block_74:
        revert(); // error_string, selector=0x8c379a0, string_length=34
        // block_75:
        revert(); // error_string, selector=0x8c379a0, string_length=34
        // block_76:
        revert(); // error_string, selector=0x8c379a0
        // block_77:
        revert(); // error_string, selector=0x8c379a0
        // block_78:
        return 1;
    }

    function private__0x13c1_0x13c1(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_18 = arg1;
          slot_19 = arg0;
          temp_0 = private__0x2a33_0x2a33(arg1, arg0, 5119);
          slot_17 = temp_0;
          if (temp_0 < 101) {
            // block_2:
            emit Event_0x38513C502B0AB4834AC1DF9502B76F75DCF7092469782CFD0DB7FE664388E25E(); // TODO: recover event signature
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=40
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1182_0x1182(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if ((arg1 ^ slot_7) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=41
          }
          else {
            // block_2:
            private__0x2315_0x2315(arg0, arg1, 444435);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x122a_0x122a(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          slot_21 = arg1;
          slot_22 = arg0;
          temp_0 = private__0x2a33_0x2a33(arg1, arg0, 4712);
          slot_20 = temp_0;
          if (temp_0 < 101) {
            // block_2:
            emit Event_0xCB5F36DF892836A2EAEDC349DE29A7581176990398EE185D16EAA8F6C1ABD8F1(); // TODO: recover event signature
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=41
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x14ab_0x14ab(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0x9D8F7706EA1113D1A167B526ECA956215946DD36CC7DF39EB16180222D8B5DF7(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x172d_0x172d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 > 4) {
            // block_2:
            temp_0 = private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(arg0, slot_2, 6097), 1000, 6107);
            slot_12 = temp_0;
            emit Event_0x12528A3C61E0F3B2D6FC707A9FC58B1AF86E252CAD0D7F4C154EBEABB162DACE(); // TODO: recover event signature
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=42
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1810_0x1810(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_2:
            private__0x22c4_0x22c4(arg0, 508171);
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x18ab_0x18ab(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x23b8_0x23b8(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2a46_0x2a46(arg1, arg0, 508469);
    }

    function private__0x23c3_0x23c3(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x2a5d_0x2a5d(arg1, arg0, 508507);
    }

    function private__0x23ce_0x23ce(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
        else {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0
          }
          else {
            // block_2:
            if (slot_0[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg0) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=38
            }
            else {
              // block_3:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2, (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1); // TODO: recover event signature
            }
          }
        }
    }

    function private__0x22c4_0x22c4(uint256 arg0, uint256 arg1) internal {
        // block_0:
        slot_5 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0 | slot_5 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_5 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0); // TODO: recover event signature
    }

    function private__0x2315_0x2315(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        emit Event_0xFFA9187BF1F18BF477BD0EA1BCBB64E93B6A98132473929EDFCE215CD9B16FAB(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg0 != 0); // TODO: recover event signature
    }

    function private__0x27a5_0x27a5(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x29d6_0x29d6(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x2a33_0x2a33(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x2a46_0x2a46(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x2a5d_0x2a5d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x2ac2_0x2ac2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x2b07_0x2b07(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(128);
    }

    function private__0x2b4a_0x2b4a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x837_0x837(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x18ab_0x18ab(arg0, arg1, uint256(uint160(msg.sender)), 316964);
        return 1;
    }

    function private__0xa23_0xa23(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x18ab_0x18ab(private__0x2a33_0x2a33(slot_1[uint256(uint160(msg.sender))], arg0, 2649), arg1, uint256(uint160(msg.sender)), 380502);
        return 1;
    }

    function private__0xb82_0xb82(uint256 arg0, uint256 arg1) internal {
        // block_0:
        uint256 temp_0;
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          if (arg0 > 1) {
            // block_2:
            temp_0 = private__0x2a5d_0x2a5d(private__0x2a46_0x2a46(arg0, slot_2, 3106), 1000, 3116);
            slot_11 = temp_0;
            emit Event_0xFF3DD5E80294197918C284BBFC3DADD97D0B40CE92106110946329088F80068A(); // TODO: recover event signature
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xd84_0xd84(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          private__0x22c4_0x22c4(0, 3511);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xdb9_0xdb9(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_5) {
          // block_1:
          emit Event_0x4B89C347592B1D537E066CB4ED98D87696AE35164745D7E370E4ADD16941DC92(arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }
}
