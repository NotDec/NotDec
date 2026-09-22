contract Decompiled {
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_0;
    uint256 public slot_1;
    mapping(uint256 => uint256) public slot_2;
    mapping(uint256 => uint256) public slot_3;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function EnemyAddress(address arg0, bool arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 == uint256(uint160(tx.origin))) {
          // block_5:
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function Pause() public returns (bool ret0) {
        // block_0:
        return slot_1 != 0;
    }

    function public_0x10c8aeac() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function isEnemy(address arg0) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_0[uint256(uint160(arg0))] != 0;
    }

    function owner() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function allowance(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_6:
        return slot_3[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function approve(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return private__0x3c8_0x3c8(arg1, uint256(uint160(arg0)), 85756) != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return slot_2[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))];
    }

    function decimals() public returns (uint256 ret0) {
        // block_0:
        return slot_5;
    }

    function decreaseAllowance(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        if (slot_3[uint256(uint160(msg.sender))] < arg1) {
          // block_6:
          revert(); // error_string, selector=0x8c379a0, string_length=37
        }
        else {
          // block_5:
          private__0x8c7_0x8c7(private__0xf35_0xf35(slot_3[uint256(uint160(msg.sender))], arg1, 150398), uint256(uint160(arg0)), uint256(uint160(msg.sender)), 1898);
          return 1;
        }
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function increaseAllowance(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return private__0x495_0x495(arg1, uint256(uint160(arg0)), 86065) != 0;
    }

    function mint(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        private__0x4cc_0x4cc(arg1, uint256(uint160(arg0)), 86112);
    }

    function name() public {
        // block_0:
    }

    function owner_2() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function recoverERC20(address arg0, uint256 arg1) public {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
    }

    function renounceOwnership() public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 == uint256(uint160(tx.origin))) {
          // block_1:
          emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0((uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8, 0); // TODO: recover event signature
          slot_8 = slot_8 & uint256(0) - (uint256(1) << uint256(160));
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setPause(bool arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_2:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 == uint256(uint160(tx.origin))) {
          // block_3:
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function symbol() public {
        // block_0:
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return slot_4;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_3:
        require(uint256(uint160(arg1)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_5:
        if (slot_3[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)] < arg2) {
          // block_7:
          revert(); // error_string, selector=0x8c379a0, string_length=40
        }
        else {
          // block_6:
          private__0x8c7_0x8c7(private__0xf35_0xf35(slot_3[uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)], arg2, 118460), uint256(uint160(msg.sender)), uint256(uint160(arg0)), 1162);
          return 1;
        }
    }

    function transferOwnership(address arg0) public {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 == uint256(uint160(tx.origin))) {
          // block_5:
          if (uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_7:
            revert(); // error_string, selector=0x8c379a0, string_length=38
          }
          else {
            // block_6:
            emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_8 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), uint256(uint160(arg0)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
            slot_8 = (uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)) | slot_8 & uint256(0) - (uint256(1) << uint256(160));
          }
        }
        else {
          // block_8:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function transfer(address arg0, uint256 arg1) public returns (bool ret0) {
        // block_0:
        require(msg.data.length > 67);
        // block_1:
        require(uint256(uint160(arg0)) & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_4:
        return private__0x774_0x774(arg1, uint256(uint160(arg0)), 86467) != 0;
    }

    function private__0x4cc_0x4cc(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_8 == uint256(uint160(tx.origin))) {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=31
          }
          else {
            // block_2:
            slot_4 = private__0xf48_0xf48(slot_4, arg0, 3300);
            emit Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(0, arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xe4d_0xe4d(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 2);
        // block_1:
    }

    function private__0x3c8_0x3c8(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x8c7_0x8c7(arg0, arg1, uint256(uint160(msg.sender)), 86829);
        return 1;
    }

    function private__0x495_0x495(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x8c7_0x8c7(private__0xf48_0xf48(slot_3[uint256(uint160(msg.sender))], arg0, 150127), arg1, uint256(uint160(msg.sender)), 118496);
        return 1;
    }

    function private__0x774_0x774(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function private__0x8c7_0x8c7(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
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

    function private__0xf35_0xf35(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0xf48_0xf48(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0xf5b_0xf5b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }
}
