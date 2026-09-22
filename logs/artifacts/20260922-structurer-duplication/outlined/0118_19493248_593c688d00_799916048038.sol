contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256[] public slot_1;
    mapping(uint256 => uint256) public slot_10;
    mapping(uint256 => uint256) public slot_11;
    uint256 public slot_12;
    uint256[] public slot_2;
    uint256 public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    mapping(uint256 => uint256) public slot_7;
    mapping(uint256 => uint256) public slot_8;
    mapping(uint256 => uint256) public slot_9;

    function public_0x0e14ebdc() public returns (uint256 ret0) {
        // block_0:
        return 200;
    }

    function public_0xcfa71593() public returns (uint256 ret0) {
        // block_0:
        return 200;
    }

    function public_0xd201b01e() public {
        // block_0:
    }

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function addLiquidity() public {
        // block_0:
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return slot_8[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x59f_0x59f(arg1, uint256(uint160(arg0)), 566940) != 0;
    }

    function automatedMarketMakerPairs(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_11[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_7[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function buyTotalFees() public returns (uint256 ret0) {
        // block_0:
        return 50;
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return 18;
    }

    function launchBlock() public returns (uint256 ret0) {
        // block_0:
        return slot_5;
    }

    function limitsInEffect() public returns (bool ret0) {
        // block_0:
        return slot_4 / uint256(256) & uint256(255) != 0;
    }

    function manualSwap(uint256 arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
    }

    function maxTransactionAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_1.length;
    }

    function maxWallet() public returns (uint256 ret0) {
        // block_0:
        return slot_2.length;
    }

    function name() public {
        // block_0:
    }

    function openTrading() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_1:
          if (slot_4 / uint256(65536) & uint256(255) == 0) {
            // block_2:
            slot_5 = block.number;
            slot_4 = slot_4 & uint256((type(uint256).max - 16711680)) | uint256(65536);
          }
          else {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=24
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function removeLimits() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_1:
          slot_4 = slot_4 & uint256((type(uint256).max - 65280));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function renounceOwnership() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_1:
          slot_0 = slot_0 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function sellTotalFees() public returns (uint256 ret0) {
        // block_0:
        return 50;
    }

    function setAutomatedMarketMakerPair(address arg0, bool arg1) public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 == uint256(uint160(msg.sender))) {
          // block_1:
          if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) == (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(844096705618839578963235662334210453947556143065)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=26
          }
          else {
            // block_2:
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function swapTokensAtAmount() public returns (uint256 ret0) {
        // block_0:
        return slot_3;
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 1000000000000000000000000;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        if (msg.data.length > 99) {
          // goto block_1
        }
        // goto block_6
        // block_1:
        if (slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == type(uint256).max) {
          // block_4:
          return 1;
        }
        if (!(slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] == type(uint256).max)) {
          // goto block_2
        }
        // goto block_4
        // block_2:
        if (slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_5:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        if (!(slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2)) {
          // goto block_3
        }
        // goto block_5
        // block_3:
        private__0xe85_0xe85(slot_8[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] - arg2, uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1619);
        // goto block_4
        // TODO: recover remaining body
    }

    function transfer(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        return private__0x7da_0x7da(arg1, uint256(uint160(arg0)), 567480) != 0;
    }

    function uniswapV2Pair() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(844096705618839578963235662334210453947556143065);
    }

    function uniswapV2Router() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(730750818665451459101842416358141509827966271487) & uint256(697323163401596485410334513241460920685086001293);
    }

    function withdrawStuckToken(address arg0, address arg1) public {
        // block_0:
    }

    function private__0x1f39_0x1f39(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x1eac_0x1eac(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 2);
        // block_1:
    }

    function private__0x1f82_0x1f82(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x1f99_0x1f99(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x207f_0x207f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x2092_0x2092(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x59f_0x59f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0xe85_0xe85(arg0, arg1, uint256(uint160(msg.sender)), 567964);
        return 1;
    }

    function private__0x7da_0x7da(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0xe85_0xe85(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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
