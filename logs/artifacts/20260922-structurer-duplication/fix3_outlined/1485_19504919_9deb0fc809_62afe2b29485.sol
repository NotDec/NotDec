contract Decompiled {
    event Event_0xD81FC9B8523134ED613870ED029D6170CBB73AA6A6BC311B9A642689FB9DF59A(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xBF551EC93859B170F9B2141BD9298BF3F64322C6F7BEB2543A0CB669834118BF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xE183F33DE2837795525B4792CA4CD60535BD77C53B7E7030060BFCF5734D6B0C();
    event Event_0xB8890EDBFC1C74692F527444645F95489C3703CC2DF42E4A366F5D06FA6CD884(uint256 indexed arg0);
    event Event_0x9AEDF5FDBA8716DB3B6705CA00150643309995D4F818A249ED6DDE6677E7792D();
    event Event_0x8C0400CFE2D1199B1A725C78960BCC2A344D869B80590D0F2BD005DB15A572CE();
    event Event_0x5DB758E995A17EC1AD84BDEF7E8C3293A0BD6179BCCE400DFF5D4C3D87DB726B();
    event Event_0xC264D91F3ADC5588250E1551F547752CA0CFA8F6B530D243B9F9F4CAB10EA8E5();
    event Event_0x9D5C7C0B934DA8FEFA9C7760C98383778A12DFBFC0C3B3106518F43FB9508AC0();
    event Event_0xFA41487AD5D6728F0B19276FA1EDDC16558578F5109FC39D2DC33C3230470DAB();

    uint256 public slot_0;
    mapping(uint256 => uint256) public slot_1;
    uint256 public slot_10;
    uint256 public slot_11;
    mapping(uint256 => uint256) public slot_2;
    mapping(uint256 => uint256) public slot_3;
    uint256 public slot_4;
    mapping(uint256 => uint256) public slot_7;
    mapping(uint256 => uint256) public slot_8;
    uint256 public slot_9;

    function NO_EXTRA_GAS() public returns (uint256 ret0) {
        // block_0:
        return 0;
    }

    function PT_SEND() public returns (uint256 ret0) {
        // block_0:
        return 0;
    }

    function public_0x0df37483() public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        private__0x1b88_0x1b88(3322);
    }

    function public_0x365260b4() public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        return (0 /* TODO: unresolved value: private.ret8 */, 0 /* TODO: unresolved value: private.ret7 */);
    }

    function public_0x3f1f4fa4() public returns (uint256 ret0) {
        // block_0:
        return slot_3[0 /* TODO: unresolved value: private__0x34dc_0x34dc */];
    }

    function public_0x695ef6bf() public {
        // block_0:
    }

    function public_0x76203b48() public {
        // block_0:
    }

    function public_0x9bdb9812() public returns (bool ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: evm_sload */ & uint256(255) != 0;
    }

    function public_0xa4c51df5() public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        return (0 /* TODO: unresolved value: private.ret11 */, 0 /* TODO: unresolved value: private.ret10 */);
    }

    function public_0xc4461834() public returns (uint256 ret0) {
        // block_0:
        return 10000;
    }

    function public_0xe6a20ae6() public returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function public_0xeaffd49a() public {
        // block_0:
        require(msg.data.length > 259);
        // block_1:
        require(!false /* TODO: evm.gt.not */);
        // block_2:
        require(!false /* TODO: evm.gt17.not */);
        // block_3:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return private__0x17d7_0x17d7(uint256(uint160(arg1)), uint256(uint160(arg0)), 915712);
    }

    function approve(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        private__0x1be2_0x1be2(arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 916309);
        return 1;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_7[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function circulatingSupply() public returns (uint256 ret0) {
        // block_0:
        return private__0x11fa_0x11fa(705912);
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x17d7_0x17d7(uint256(uint160(arg0)), uint256(uint160(msg.sender)), 4926);
        if (temp_0 < arg1) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_1:
          private__0x1be2_0x1be2(temp_0 - arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 916817);
          return 1;
        }
    }

    function failedMessages(uint16 arg0, bytes calldata arg1, uint64 arg2) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function forceResumeReceive(uint16 arg0, bytes calldata arg1) public {
        // block_0:
    }

    function getConfig(uint16 arg0, uint16 arg1, address arg2, uint256 arg3) public {
        // block_0:
        require(msg.data.length > 131);
        // block_1:
        private__0x34f7_0x34f7(uint256(uint160(arg2)), 15255);
    }

    function getTrustedRemoteAddress(uint16 arg0) public {
        // block_0:
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        private__0x1be2_0x1be2(private__0x3c02_0x3c02(private__0x17d7_0x17d7(uint256(uint160(arg0)), uint256(uint160(msg.sender)), 3573), arg1, 3583), uint256(uint160(arg0)), uint256(uint160(msg.sender)), 916392);
        return 1;
    }

    function isTrustedRemote(uint16 arg0, bytes calldata arg1) public returns (bool ret0) {
        // block_0:
        if (false /* TODO: evm.iszero9 */) {
          // goto block_5
        }
        // block_1:
        if (!false /* TODO: evm.lt */) {
          // goto block_4
        }
        // block_2:
        while (false /* TODO: evm.gt */) {
          // block_3:
        }
        // block_4:
        // block_5:
        return 0 /* TODO: unresolved value: evm_sha3 */ == 0 /* TODO: unresolved value: evm_sha3 */;
    }

    function lzEndpoint() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(586043879749118965183522797960821433037940905589);
    }

    function lzReceive(uint16 arg0, bytes calldata arg1, uint64 arg2, bytes calldata arg3) public {
        // block_0:
    }

    function minDstGasLookup(uint16 arg0, uint16 arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_2[arg0];
    }

    function name() public {
        // block_0:
    }

    function nonblockingLzReceive(uint16 arg0, bytes calldata arg1, uint64 arg2, bytes calldata arg3) public {
        // block_0:
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function precrime() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_4 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function renounceOwnership() public {
        // block_0:
        private__0x109d_0x109d(705668);
    }

    function retryMessage(uint16 arg0, bytes calldata arg1, uint64 arg2, bytes calldata arg3) public {
        // block_0:
        if (false /* TODO: evm.branch.cond.not */) {
          // block_4:
          revert(); // error_string, selector=0x8c379a0, string_length=35
        }
        else {
          // block_1:
          if (false /* TODO: evm.eq */) {
            // block_2:
            emit Event_0xC264D91F3ADC5588250E1551F547752CA0CFA8F6B530D243B9F9F4CAB10EA8E5(); // TODO: recover event signature
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=33
          }
        }
    }

    function setConfig(uint16 arg0, uint16 arg1, uint256 arg2, bytes calldata arg3) public {
        // block_0:
    }

    function setMinDstGas(uint16 arg0, uint16 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x1b88_0x1b88(6154);
        emit Event_0x9D5C7C0B934DA8FEFA9C7760C98383778A12DFBFC0C3B3106518F43FB9508AC0(); // TODO: recover event signature
    }

    function setPrecrime(address arg0) public {
        // block_0:
        private__0x1b88_0x1b88(5347);
        slot_4 = uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) | slot_4 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x5DB758E995A17EC1AD84BDEF7E8C3293A0BD6179BCCE400DFF5D4C3D87DB726B(); // TODO: recover event signature
    }

    function setReceiveVersion(uint16 arg0) public {
        // block_0:
    }

    function setSendVersion(uint16 arg0) public {
        // block_0:
    }

    function setTrustedRemoteAddress(uint16 arg0, bytes calldata arg1) public {
        // block_0:
        private__0x1b88_0x1b88(5186);
        emit Event_0x8C0400CFE2D1199B1A725C78960BCC2A344D869B80590D0F2BD005DB15A572CE(); // TODO: recover event signature
    }

    function setTrustedRemote(uint16 arg0, bytes calldata arg1) public {
        // block_0:
        private__0x1b88_0x1b88(6547);
        emit Event_0xFA41487AD5D6728F0B19276FA1EDDC16558578F5109FC39D2DC33C3230470DAB(); // TODO: recover event signature
    }

    function sharedDecimals() public returns (uint256 ret0) {
        // block_0:
        return 6;
    }

    function supportsInterface(bytes4 arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xb88_0xb88 */;
    }

    function symbol() public {
        // block_0:
    }

    function token() public returns (uint256 ret0) {
        // block_0:
        return uint256(uint160(address(this))) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x34f7_0x34f7(uint256(uint160(arg0)), 13684);
        private__0x34f7_0x34f7(uint256(uint160(arg1)), 13700);
        private__0x1d06_0x1d06(arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 3447);
        private__0x1d80_0x1d80(arg2, uint256(uint160(arg1)), uint256(uint160(arg0)), 916350);
        return 1;
    }

    function transferOwnership(address arg0) public {
        // block_0:
        private__0x19e5_0x19e5(uint256(uint160(arg0)), 915905);
    }

    function transfer(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        private__0x1d80_0x1d80(arg1, uint256(uint160(arg0)), uint256(uint160(msg.sender)), 916903);
        return 1;
    }

    function trustedRemoteLookup(uint16 arg0) public {
        // block_0:
    }

    function private__0x109d_0x109d(uint256 arg0) internal {
        // block_0:
        private__0x1b88_0x1b88(4261);
        private__0x218e_0x218e(0, 916551);
    }

    function private__0x19e5_0x19e5(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0x1b88_0x1b88(6637);
        if (arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=38
        }
        else {
          // block_1:
          private__0x218e_0x218e(arg0, 917074);
        }
    }

    function private__0x11fa_0x11fa(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function private__0x218e_0x218e(uint256 arg0, uint256 arg1) internal {
        // block_0:
        slot_0 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0 | slot_0 & uint256(0) - (uint256(1) << uint256(160));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), (uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg0); // TODO: recover event signature
    }

    function private__0x17d7_0x17d7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_8[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1];
    }

    function private__0x1b88_0x1b88(uint256 arg0) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=32
        }
    }

    function private__0x1be2_0x1be2(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0x1d06_0x1d06(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x17d7_0x17d7(arg1, arg2, 7442);
        if (!(temp_0 == type(uint256).max)) {
          // block_2:
          if (temp_0 < arg0) {
            // block_4:
            revert(); // error_string, selector=0x8c379a0, string_length=29
          }
          // block_3:
          private__0x1be2_0x1be2(temp_0 - arg0, arg1, arg2, 917386);
        }
        // block_1:
    }

    function private__0x1d80_0x1d80(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_5:
            revert(); // error_string, selector=0x8c379a0, string_length=35
          }
          else {
            // block_2:
            if (slot_7[arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg0) {
              // block_4:
              revert(); // error_string, selector=0x8c379a0, string_length=38
            }
            else {
              // block_3:
              emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            }
          }
        }
    }

    function private__0x24a2_0x24a2(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (!(arg2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == uint256(uint160(address(this))))) {
          // block_1:
          if (!((uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg2 == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)))) {
            // block_2:
            private__0x1d06_0x1d06(arg0, uint256(uint160(msg.sender)), arg2, 9440);
          }
        }
        // block_3:
        private__0x1d80_0x1d80(arg0, arg1, arg2, 9451);
        return arg0;
    }

    function private__0x257e_0x257e(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5) internal {
        // block_0:
        emit Event_0xE183F33DE2837795525B4792CA4CD60535BD77C53B7E7030060BFCF5734D6B0C(); // TODO: recover event signature
    }

    function private__0x2620_0x2620(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (arg0 / uint256(1000000000000) > (uint256(1) << uint256(64)) + uint256(type(uint256).max)) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=26
        }
        else {
          // block_1:
          return arg0 / uint256(1000000000000);
        }
    }

    function private__0x2d53_0x2d53(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 & (uint256(1) << uint256(64)) + uint256(type(uint256).max)) * uint256(1000000000000);
    }

    function private__0x2d88_0x2d88(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=31
        }
        else {
          // block_1:
          slot_9 = private__0x3c02_0x3c02(slot_9, arg0, 12666);
          emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(0, arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          return arg0;
        }
    }

    function private__0x34f7_0x34f7(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x36a2_0x36a2(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 + uint256(31) & uint256((type(uint256).max - 31))) + uint256(32);
    }

    function private__0x3bdc_0x3bdc(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x3c02_0x3c02(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x3c1a_0x3c1a(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        return arg2 + (arg1 + uint256(31) & uint256((type(uint256).max - 31))) + uint256(32);
    }

    function private__0x3c43_0x3c43(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal returns (uint256 ret0) {
        // block_0:
        return private__0x3c1a_0x3c1a(arg2, arg1, arg0 + uint256(64), 918955);
    }

    function private__0x3ca7_0x3ca7(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x3ce4_0x3ce4(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6) internal returns (uint256 ret0) {
        // block_0:
        return private__0x3c1a_0x3c1a(arg2, arg1, arg0 + uint256(128), 919033);
    }

    function private__0x3d58_0x3d58(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6, uint256 arg7, uint256 arg8, uint256 arg9) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x3c1a_0x3c1a(arg7, arg6, arg0 + uint256(192), 15734);
        return private__0x3c1a_0x3c1a(arg2, arg1, temp_0, 15780);
    }

    function private__0xb88_0xb88(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(528403959) << uint256(224) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_1:
          return 1;
        }
        else {
          // block_2:
          return arg0 & uint256(0) - (uint256(1) << uint256(224)) == uint256(33540519) << uint256(224) ? 1 : 0;
        }
    }

    function private__0xf56_0xf56(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4, uint256 arg5, uint256 arg6) internal {
        // block_0:
        if (uint256(uint160(address(this))) == uint256(uint160(msg.sender))) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=38
        }
    }
}
