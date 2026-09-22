contract Decompiled {
    event Event_0xE1FFFCC4923D04B559F4D29A8BFC6CDA04EB5B0D3C460751C2402C5C5CC9109C(uint256 indexed arg0);
    event Event_0xA3F1EE9126A074D9326C682F561767F710E927FAA811F7A99829D49DC421797A();
    event Event_0x4A504A94899432A9846E1AA406DCEB1BCFD538BB839071D49D1E5E23F5BE30EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xC0BA8FE4B176C1714197D43B9CC6BCF797A4A7461C5FE8D0EF6E184AE7601E51(uint256 indexed arg0);
    event Event_0x526441BB6C1ABA3C9A4A6CA1D6545DA9C2333C8C48343EF398EB858D72B79236(uint256 indexed arg0);
    event Event_0x33E13ECB54C3076D8E8BB8C2881800A4D972B792045FFAE98FDF46DF365FED75(uint256 indexed arg0);
    event Event_0x8001553A916EF2F495D26A907CC54D96ED840D7BDA71E73194BF5A9DF7A76B90(uint256 indexed arg0);
    event Event_0xF6A317157440607F36269043EB55F1287A5A19BA2216AFEAB88CD46CBCFB88E9(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xF39E6E1EB0EDCF53C221607B54B00CD28F3196FED0A24994DC308B8F611B682D(uint256 indexed arg0);
    event Event_0xC71BDC6AFAF9B1AA90A7078191D4FC1ADF3BF680FCA3183697DF6B0DC226BCA2();

    mapping(uint256 => uint256) public slot_0;
    mapping(uint256 => uint256) public slot_1;
    mapping(uint256 => uint256) public slot_2;
    uint256[] public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;

    function MAX_OWNER_COUNT() public returns (uint256 ret0) {
        // block_0:
        return 50;
    }

    function addOwner(address arg0) public {
        // block_0:
        require((uint256(uint160(msg.sender)) ^ uint256(uint160(address(this)))) & uint256(1461501637330902918203684832716283019655932542975) == 0);
        // block_1:
        require(slot_2[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255) == 0);
        // block_2:
        require(uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975) != 0);
        // block_3:
        require(false /* TODO: _0xfac_0x0.in */);
        // block_4:
        emit Event_0xF39E6E1EB0EDCF53C221607B54B00CD28F3196FED0A24994DC308B8F611B682D(uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
    }

    function calcMaxWithdraw() public returns (uint256 ret0) {
        // block_0:
        return private__0xdec_0xdec(811);
    }

    function changeDailyLimit(uint256 arg0) public {
        // block_0:
        require((uint256(uint160(msg.sender)) ^ uint256(uint160(address(this)))) & uint256(1461501637330902918203684832716283019655932542975) == 0);
        // block_1:
        slot_6 = arg0;
        emit Event_0xC71BDC6AFAF9B1AA90A7078191D4FC1ADF3BF680FCA3183697DF6B0DC226BCA2(); // TODO: recover event signature
    }

    function changeRequirement(uint256 arg0) public {
        // block_0:
    }

    function confirmTransaction(uint256 arg0) public {
        // block_0:
        private__0x17b1_0x17b1(arg0, 1894);
    }

    function confirmations(uint256 arg0, address arg1) public returns (bool ret0) {
        // block_0:
        return slot_1[arg0] / uint256(256) ** uint256(0) & uint256(255) != 0;
    }

    function dailyLimit() public returns (uint256 ret0) {
        // block_0:
        return slot_6;
    }

    function executeTransaction(uint256 arg0) public {
        // block_0:
    }

    function fallback() public {
        // block_0:
        if (!false /* TODO: evm.gt.not */) {
          // block_1:
          emit Event_0xE1FFFCC4923D04B559F4D29A8BFC6CDA04EB5B0D3C460751C2402C5C5CC9109C(uint256(uint160(msg.sender)) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        }
        // block_2:
    }

    function getConfirmationCount(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x11af_0x11af */;
    }

    function getConfirmations(uint256 arg0) public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
        }
        // block_2:
    }

    function getOwners() public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
        }
        // block_2:
    }

    function getTransactionCount(bool arg0, bool arg1) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xe29_0xe29 */;
    }

    function getTransactionIds(uint256 arg0, uint256 arg1, bool arg2, bool arg3) public {
        // block_0:
        while (false /* TODO: evm.lt.not */) {
          // block_3:
        }
        // block_2:
    }

    function isConfirmed(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x10c9_0x10c9 */;
    }

    function isOwner(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_2[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255) != 0;
    }

    function lastDay() public returns (uint256 ret0) {
        // block_0:
        return slot_7;
    }

    function owners(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_3[arg0] / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function removeOwner(address arg0) public {
        // block_0:
        require((uint256(uint160(msg.sender)) ^ uint256(uint160(address(this)))) & uint256(1461501637330902918203684832716283019655932542975) == 0);
        if ((uint256(uint160(msg.sender)) ^ uint256(uint160(address(this)))) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // goto block_1
        }
        // goto block_11
        // block_1:
        if (slot_2[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255) == 0) {
          // goto block_10
        }
        // goto block_2
        while (!false /* TODO: evm.eq44.not */) {
          // block_2:
          // block_3:
          if (!false /* TODO: evm.lt.not */) {
            // goto block_7
          }
          // goto block_4
        }
        // block_6:
        // block_7:
        // block_6:
        // block_7:
        if (!(slot_4 > slot_3.length)) {
          // goto block_9
        }
        // block_8:
        // block_9:
        emit Event_0x8001553A916EF2F495D26A907CC54D96ED840D7BDA71E73194BF5A9DF7A76B90(uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        // block_10:
        revert(); // empty
    }

    function replaceOwner(address arg0, address arg1) public {
        // block_0:
        require((uint256(uint160(msg.sender)) ^ uint256(uint160(address(this)))) & uint256(1461501637330902918203684832716283019655932542975) == 0);
        // block_1:
        require(slot_2[uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255) != 0);
        // block_2:
        // block_8:
        revert(); // empty
        if (slot_2[uint256(uint160(arg1)) & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255) == 0) {
          // goto block_3
        }
        else {
          // goto block_8
        }
        while (!false /* TODO: evm.eq53.not */) {
          // block_5:
          // block_3:
          if (!false /* TODO: evm.lt.not */) {
            // goto block_7
          }
          // goto block_4
        }
        // block_6:
        // block_7:
        emit Event_0x8001553A916EF2F495D26A907CC54D96ED840D7BDA71E73194BF5A9DF7A76B90(uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        emit Event_0xF39E6E1EB0EDCF53C221607B54B00CD28F3196FED0A24994DC308B8F611B682D(uint256(uint160(arg1)) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        // block_6:
        // block_7:
        emit Event_0x8001553A916EF2F495D26A907CC54D96ED840D7BDA71E73194BF5A9DF7A76B90(uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        emit Event_0xF39E6E1EB0EDCF53C221607B54B00CD28F3196FED0A24994DC308B8F611B682D(uint256(uint160(arg1)) & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        // TODO: recover remaining body
    }

    function required() public returns (uint256 ret0) {
        // block_0:
        return slot_4;
    }

    function revokeConfirmation(uint256 arg0) public {
        // block_0:
        require(slot_2[uint256(uint160(msg.sender)) & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255) != 0);
        // block_1:
        require(slot_1[arg0] / uint256(256) ** uint256(0) & uint256(255) != 0);
        // block_2:
        require(slot_0[arg0] / uint256(256) ** uint256(0) & uint256(255) == 0);
        // block_3:
        emit Event_0xF6A317157440607F36269043EB55F1287A5A19BA2216AFEAB88CD46CBCFB88E9(uint256(uint160(msg.sender)) & uint256(1461501637330902918203684832716283019655932542975), arg0); // TODO: recover event signature
    }

    function spentToday() public returns (uint256 ret0) {
        // block_0:
        return slot_8;
    }

    function submitTransaction(address arg0, uint256 arg1, bytes calldata arg2) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x198e_0x198e */;
    }

    function transactionCount() public returns (uint256 ret0) {
        // block_0:
        return slot_5;
    }

    function transactions(uint256 arg0) public {
        // block_0:
        if (false /* TODO: evm.iszero39 */) {
          // goto block_6
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
        // block_6:
        // TODO: recover remaining body
    }

    function private__0x17b1_0x17b1(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(slot_2[uint256(uint160(msg.sender)) & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255) != 0);
        // block_1:
        require(slot_0[arg0] / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975) != 0);
        // block_2:
        require(slot_1[arg0] / uint256(256) ** uint256(0) & uint256(255) == 0);
        // block_3:
        emit Event_0x4A504A94899432A9846E1AA406DCEB1BCFD538BB839071D49D1E5E23F5BE30EF(uint256(uint160(msg.sender)) & uint256(1461501637330902918203684832716283019655932542975), arg0); // TODO: recover event signature
    }

    function private__0x198e_0x198e(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        require(arg2 & uint256(1461501637330902918203684832716283019655932542975) != 0);
        // block_1:
        slot_5 = slot_5 + uint256(1);
        emit Event_0xC0BA8FE4B176C1714197D43B9CC6BCF797A4A7461C5FE8D0EF6E184AE7601E51(slot_5); // TODO: recover event signature
        private__0x17b1_0x17b1(slot_5, 6566);
        return slot_5;
    }

    function private__0xdec_0xdec(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        if (block.timestamp > slot_7 + uint256(86400)) {
          // block_4:
          return slot_6;
        }
        else {
          // block_1:
          if (slot_6 < slot_8) {
            // block_3:
            return 0;
          }
          else {
            // block_2:
            return slot_6 - slot_8;
          }
        }
    }
}
