contract Decompiled {
    uint256 public slot_0;
    uint256 public slot_1;
    uint256 public slot_2;

    function public_0x0d73ed7b() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_2:
        return 0 /* TODO: unresolved value: private__0x291_0x291 */;
    }

    function public_0x4e90045b() public returns (uint256 ret0) {
        // block_0:
        return slot_1;
    }

    function public_0x58aa1c8b() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_2) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // custom_error_candidate, selector=0x90977503, args=0
        }
    }

    function public_0x9b9059ff() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(603297494131945452531224366415988148891762667298);
    }

    function public_0x9fe32195() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1393060696023666239917003968693327223581040371852);
    }

    function public_0xab4766e8() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(544110644340816939372220746866710871299132720153);
    }

    function public_0xb550c7ba() public returns (uint256 ret0) {
        // block_0:
        return slot_0;
    }

    function public_0xd0f3230b() public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3, uint256 ret4) {
        // block_0:
    }

    function public_0xd487a901() public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_2) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // custom_error_candidate, selector=0x90977503, args=0
        }
    }

    function public_0xe6130f33() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(503463888201465655360223080589361039510797078708);
    }

    function public_0xe663fef9() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(983855839603266185103516034695175458738108255897);
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function gov() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_2 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function latestAnswer() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x3dc_0x3dc */;
    }

    function latestRoundData() public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3, uint256 ret4) {
        // block_0:
    }

    function setGov(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        private__0x867_0x867(uint256(uint160(arg0)), 160524);
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_2) {
          // block_3:
          slot_2 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_2 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_4:
          revert(); // custom_error_candidate, selector=0x90977503, args=0
        }
    }

    function private__0x867_0x867(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }
}
