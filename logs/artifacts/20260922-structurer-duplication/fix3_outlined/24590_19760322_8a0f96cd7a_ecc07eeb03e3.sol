contract Decompiled {
    event Event_0x3673F408E6EC017E835B025745B339FA7BED8EAF1A1916941DD9A60FFCCFC71E();
    event Event_0x14EA8B2DCD1DAD93E7F7974CDB2FCD29AB67C45CA7F0879AAEABB932A0C52B88();
    event Event_0x3853B2CABD38DB656EEC73CFA8DA872697C2B82A1A131B0AF1A5A177C895A688();
    event Event_0xF236E35FA66B9259CF42760562BE24EF7B5A827125803D8E3AD3FB6136D074AF();
    event Event_0xC7F505B2F371AE2175EE4913F4499E1F2633A7B5936321EED1CDAEB6115181D2();
    event Event_0x70CD0C14252EDEF0440009634A4E86036A1747C9FBE7E4C2C05B8B62F9C410E2();
    event Event_0xD2D636EFCAD0CEA42E170256F4C5D8B1CD81E47B855557EDAF44014E6CC4CEE6();
    event Event_0x5D611F318680D00598BB735D61BACF0C514C6B50E1E5AD30040A4DF2B12791C7(uint256 indexed arg0);
    event Event_0xDAA2B90D4F4E53AAC69BC06E8CE6FC4A0D69C6582F74CD6E4547ABCB972F7C6E();
    event Event_0x3EF8564460ADA92419608D823C014975D98F8104D7D1E68C222967AC6814CD97();

    uint256[] public slot_1;
    uint256[] public slot_2;
    uint256[] public slot_3;
    uint256[] public slot_4;

    function public_0x04e9f64c() public {
        // block_0:
    }

    function public_0x0b9e332a() public {
        // block_0:
    }

    function public_0x5f406ec2() public {
        // block_0:
    }

    function public_0x926fee8d() public {
        // block_0:
    }

    function public_0xba0f2637() public returns (uint256 ret0) {
        // block_0:
        require(msg.data.length > 35);
        // block_1:
        return 0 /* TODO: unresolved value: private__0x5ad_0x5ad */;
    }

    function public_0xbc8ea8fd() public {
        // block_0:
    }

    function public_0xfeabd094() public {
        // block_0:
    }

    function eip712Domain() public {
        // block_0:
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function getImplementation() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & 0 /* TODO: unresolved value: evm_sload */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function initialize(address arg0) public {
        // block_0:
    }

    function isTrustedForwarder(address arg0) public returns (bool ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x3f5_0x3f5 */ != 0;
    }

    function simulateUpgradeTo(address arg0) public {
        // block_0:
        if (!false /* TODO: evm.eq */) {
          // block_1:
        }
        // block_2:
        if (false /* TODO: evm.iszero */) {
          // goto block_5
        }
        // block_3:
        if (!false /* TODO: evm.eq41.not */) {
          // goto block_5
        }
        // block_4:
        revert(); // empty
        // block_5:
        revert(); // custom_error_candidate, selector=0xa1cfa5a8, args=0
    }

    function upgradeTo(address arg0) public {
        // block_0:
    }

    function private__0x1196_0x1196(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x11a0_0x11a0(uint256 arg0) internal {
        // block_0:
    }

    function private__0x185d_0x185d(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_1[arg1] != 0 ? 1 : 0;
    }

    function private__0x187f_0x187f(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1cf1_0x1cf1(arg0 & (uint256(1) << uint256(160)) + uint256(type(uint256).max), arg1, 314806);
    }

    function private__0x21a8_0x21a8(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0x211c_0x211c(arg1, arg0 + uint256(32), 315775);
        return arg0 + uint256(256);
    }

    function private__0x21bd_0x21bd(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal returns (uint256 ret0) {
        // block_0:
        private__0x211c_0x211c(arg3, arg0, 8652);
        return arg0 + (arg1 + uint256(31) & uint256((type(uint256).max - 31))) + uint256(288);
    }

    function private__0x224e_0x224e(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x1cf1_0x1cf1(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        if (slot_1[arg1] == 0) {
          // block_2:
          return 1;
        }
        else {
          // block_1:
          return 0;
        }
    }

    function private__0x1f14_0x1f14(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x210a_0x210a(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 < 4294967296);
        // block_1:
    }

    function private__0x211c_0x211c(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }
}
