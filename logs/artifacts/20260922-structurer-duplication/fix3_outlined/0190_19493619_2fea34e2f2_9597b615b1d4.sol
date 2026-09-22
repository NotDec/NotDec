contract Decompiled {
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(uint256 indexed arg0, uint256 indexed arg1);

    uint256 public slot_0;
    uint256 public slot_1;
    mapping(uint256 => uint256) public slot_2;
    mapping(uint256 => uint256) public slot_3;
    mapping(uint256 => uint256) public slot_4;
    mapping(uint256 => uint256) public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;

    function public_0xeeeeeeee() public {
        // block_0:
    }

    function adminMint(address arg0) public {
        // block_0:
        private__0x9b9_0x9b9(uint256(uint160(arg0)), 996);
    }

    function airDropMint(address arg0, uint256 arg1) public {
        // block_0:
        private__0x8cf_0x8cf(arg1, uint256(uint160(arg0)), 869);
    }

    function approve(address arg0, uint256 arg1) public {
        // block_0:
        private__0x69c_0x69c(arg1, uint256(uint160(arg0)), 560);
    }

    function balanceOf(address arg0) public {
        // block_0:
        if (uint256(uint160(arg0)) & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x89c62b64
        }
        else {
          // block_1:
        }
    }

    function baseUri() public {
        // block_0:
    }

    function changeBaseUri(string calldata arg0) public {
        // block_0:
    }

    function currentTokenId() public {
        // block_0:
    }

    function getApproved(uint256 arg0) public {
        // block_0:
    }

    function isApprovedForAll(address arg0, address arg1) public {
        // block_0:
    }

    function name() public {
        // block_0:
    }

    function ownerOf(uint256 arg0) public {
        // block_0:
    }

    function owner() public {
        // block_0:
    }

    function renounceOwnership() public {
        // block_0:
        private__0x8bb_0x8bb(828);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        private__0x7b4_0x7b4(arg2, uint256(uint160(arg1)), uint256(uint160(arg0)), 642);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2, bytes calldata arg3) public {
        // block_0:
    }

    function setApprovalForAll(address arg0, bool arg1) public {
        // block_0:
    }

    function supportsInterface(bytes4 arg0) public {
        // block_0:
    }

    function symbol() public {
        // block_0:
    }

    function tokenURI(uint256 arg0) public {
        // block_0:
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
    }

    function transferOwnership(address arg0) public {
        // block_0:
        private__0xb8e_0xb8e(uint256(uint160(arg0)), 1284);
    }

    function private__0x15b1_0x15b1(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return slot_2[arg0] / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x1cda_0x1cda(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1ccb_0x1ccb(arg1, arg0, 7407);
        return arg0 + uint256(32);
    }

    function private__0x1b58_0x1b58(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 != 0 ? 1 : 0;
    }

    function private__0x1b73_0x1b73(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(32);
    }

    function private__0x1bd4_0x1bd4(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(31) & uint256((type(uint256).max - 31));
    }

    function private__0x1a8b_0x1a8b(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x1a95_0x1a95(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x1aa4_0x1aa4(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        private__0x1a95_0x1a95(arg1, arg0, 6841);
        return arg0 + uint256(32);
    }

    function private__0x1aff_0x1aff(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(26959946667150639794667015087019630673637144422540572481103610249215) == 0);
        // block_1:
    }

    function private__0x1c40_0x1c40(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x1a8b_0x1a8b(arg0, 7241));
        // block_1:
    }

    function private__0x1cb9_0x1cb9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x1ccb_0x1ccb(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x1cf5_0x1cf5(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 == private__0x1cb9_0x1cb9(arg0, 7422));
        // block_1:
    }

    function private__0x1e6a_0x1e6a(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0xf77_0xf77(uint256 arg0) internal {
        // block_0:
        if (private__0x8fd_0x8fd(3997) == private__0xd43_0xd43(3967) & uint256(1461501637330902918203684832716283019655932542975)) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // custom_error_candidate, selector=0x118cdaa7
        }
    }

    function private__0xffe_0xffe(uint256 arg0, uint256 arg1) internal {
        // block_0:
        slot_6 = (arg0 & uint256(1461501637330902918203684832716283019655932542975)) * uint256(256) ** uint256(0) | slot_6 & ~(uint256(256) ** uint256(0) * uint256(1461501637330902918203684832716283019655932542975));
        emit Event_0x8BE0079C531659141344CD1FD0A4F28419497F9722A3DAAFE3B4186F6B6457E0(slot_6 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975), arg0 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
    }

    function private__0x218f_0x218f(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal returns (uint256 ret0) {
        // block_0:
        private__0x1ccb_0x1ccb(arg3, arg0, 8612);
        private__0x1a95_0x1a95(arg2, arg0 + uint256(32), 8625);
        private__0x1ccb_0x1ccb(arg1, arg0 + uint256(64), 8638);
        return arg0 + uint256(96);
    }

    function private__0x21db_0x21db(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 + uint256(31)) / uint256(32);
    }

    function private__0x21eb_0x21eb(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 << arg0;
    }

    function private__0x2254_0x2254(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x1a8b_0x1a8b(private__0x1a8b_0x1a8b(arg0, 8805), 8815);
    }

    function private__0x22aa_0x22aa(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x22e5_0x22e5(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (!(arg1 < 32)) {
          // block_2:
        }
        // block_1:
    }

    function private__0x2473_0x2473(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x1a8b_0x1a8b(arg0, 9342);
        return temp_0 + uint256(1);
    }

    function private__0x24bb_0x24bb(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0;
    }

    function private__0x2707_0x2707(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal returns (uint256 ret0) {
        // block_0:
        private__0x1ccb_0x1ccb(arg2, arg0, 10012);
        private__0x1a95_0x1a95(arg1, arg0 + uint256(32), 10025);
        return arg0 + uint256(64);
    }

    function private__0x50c_0x50c(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == (type(uint256).max - 57591534043048631271602713168962198724648561309353517944097662808962438791167) || arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == 41326688264978518566617017154793646184809722027297161851759941631668658372608 || arg0 & uint256((type(uint256).max - 26959946667150639794667015087019630673637144422540572481103610249215)) == 904250603428552709895185118199468575982109441609966099573332780532423983104 ? 1 : 0;
    }

    function private__0x680_0x680(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        private__0xc7e_0xc7e(arg0, 1675);
        return private__0xd06_0xd06(arg0, 1685);
    }

    function private__0x69c_0x69c(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0xd4b_0xd4b(private__0xd43_0xd43(1705), arg0, arg1, 1710);
    }

    function private__0x7b4_0x7b4(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x7d4_0x7d4(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0xf77_0xf77(2012);
    }

    function private__0x7ef_0x7ef(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0xc7e_0xc7e(arg0, 2042);
    }

    function private__0x8bb_0x8bb(uint256 arg0) internal {
        // block_0:
        private__0xf77_0xf77(2243);
        private__0xffe_0xffe(0, 2253);
    }

    function private__0x8cf_0x8cf(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0xf77_0xf77(2263);
        slot_7 = private__0x2473_0x2473(slot_7, 2282);
    }

    function private__0x8fd_0x8fd(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return slot_6 / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0x9b9_0x9b9(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0xf77_0xf77(2497);
        slot_7 = private__0x2473_0x2473(slot_7, 2516);
    }

    function private__0xa76_0xa76(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0xd43_0xd43(2689);
        if (arg1 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x5b08ba18
        }
        else {
          // block_1:
          emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(temp_0 & uint256(1461501637330902918203684832716283019655932542975), arg1 & uint256(1461501637330902918203684832716283019655932542975)); // TODO: recover event signature
        }
    }

    function private__0xa8c_0xa8c(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
    }

    function private__0xafa_0xafa(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_5[arg1 & uint256(1461501637330902918203684832716283019655932542975)] / uint256(256) ** uint256(0) & uint256(255);
    }

    function private__0xb8e_0xb8e(uint256 arg0, uint256 arg1) internal {
        // block_0:
        private__0xf77_0xf77(2966);
        if (arg0 & uint256(1461501637330902918203684832716283019655932542975) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x1e4fbdf7
        }
        else {
          // block_1:
          private__0xffe_0xffe(arg0, 3089);
        }
    }

    function private__0xc7e_0xc7e(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        uint256 temp_0;
        temp_0 = private__0x15b1_0x15b1(arg0, 3210);
        if (temp_0 == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x7e273289
        }
        else {
          // block_1:
          return temp_0;
        }
    }

    function private__0xd06_0xd06(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return slot_4[arg0] / uint256(256) ** uint256(0) & uint256(1461501637330902918203684832716283019655932542975);
    }

    function private__0xd43_0xd43(uint256 arg0) internal returns (uint256 ret0) {
        // block_0:
        return uint256(uint160(msg.sender));
    }

    function private__0xd4b_0xd4b(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }
}
