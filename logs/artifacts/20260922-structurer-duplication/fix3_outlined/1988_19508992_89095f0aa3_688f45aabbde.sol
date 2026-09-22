contract Decompiled {
    event Event_0x5A272403B402D892977DF56625F4164CCAF70CA3863991C43ECFE76A6905B0A1();
    event Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xDDF252AD1BE2C89B69C2B068FC378DAA952BA7F163C4A11628F55A4DF523B3EF(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0xC7F505B2F371AE2175EE4913F4499E1F2633A7B5936321EED1CDAEB6115181D2();
    event Event_0x8C5BE1E5EBEC7D5BD14F71427D1E84F3DD0314C0F7B2291E5B200AC8C7C3B925(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x4D54CCAF27A8BE42C0D9691E765FA614353731AB09C493ECB4822F69CC602061();
    event Event_0x78F78EAD0839CBD5C234B44DDD6D28B52BDEC2C1016C8050BBE5557DD3EAC379();

    uint256 public slot_0;
    mapping(uint256 => uint256) public slot_1;
    mapping(uint256 => uint256) public slot_10;
    uint256 public slot_2;
    uint256 public slot_4;
    uint256 public slot_5;
    uint256 public slot_6;
    uint256 public slot_7;
    uint256 public slot_8;
    uint256 public slot_9;

    function public_0x45b1b008() public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function public_0x7cf707fc() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_6 & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function public_0x7e7ac4ca() public {
        // block_0:
        if (!(uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536))) {
          // goto block_4
        }
        while (!false /* TODO: evm.gt */) {
          // block_3:
        }
        // block_2:
        // block_4:
        revert(); // error_string, selector=0x8c379a0
    }

    function public_0xb56e21a8() public returns (uint256 ret0) {
        // block_0:
        return slot_8;
    }

    function public_0xc8ace705() public returns (uint256 ret0) {
        // block_0:
        require(false /* TODO: evm.lt */);
        // block_1:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function admin() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function approve(address arg0, uint256 arg1) public {
        // block_0:
        private__0x8a0_0x8a0(arg1, uint256(uint160(arg0)), 509074);
    }

    function auth(address arg0) public returns (bool ret0) {
        // block_0:
        return slot_1[uint256(uint160(arg0))] != 0;
    }

    function balanceOf(address arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x123a_0x123a */;
    }

    function baseExtension() public {
        // block_0:
    }

    function baseURI() public {
        // block_0:
    }

    function batchMint(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        if (false /* TODO: evm.iszero */) {
          // block_1:
        }
        // block_2:
        if (!false /* TODO: _0xcb8_0x0.in.reload */) {
          // goto block_3
        }
        // goto block_4
        // block_3:
        // goto block_4
        while (false /* TODO: evm.lt.not */) {
          // block_8:
        }
        // block_7:
        return 1;
        // block_4:
        require(!false /* TODO: _0xcd2_0x0.reload */, "onlyExternal");
        // block_5:
        // block_9:
        revert(); // error_string, selector=0x8c379a0, string_length=25
        if (false /* TODO: evm.eq11 */) {
          // goto block_6
        }
        else {
          // goto block_9
        }
        // TODO: recover remaining body
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function getApproved(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x88b_0x88b */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function getNFTs(address arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
    }

    function initialize(string calldata arg0, string calldata arg1) public {
        // block_0:
    }

    function isApprovedForAll(address arg0, address arg1) public returns (bool ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x1572_0x1572 */ != 0;
    }

    function isPaused() public returns (bool ret0) {
        // block_0:
        return slot_0 != 0;
    }

    function mintPrice() public returns (uint256 ret0) {
        // block_0:
        return slot_7;
    }

    function mint(address arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x104e_0x104e */;
    }

    function name() public {
        // block_0:
    }

    function ownerOf(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x1043_0x1043 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function royaltyInfo(uint256 arg0, uint256 arg1) public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        if (false /* TODO: evm.branch.cond7.not */) {
          // block_1:
        }
        // block_2:
        return (0 /* TODO: unresolved value: evm.mload27 */ & (uint256(1) << uint256(160)) + uint256(type(uint256).max), 0 /* TODO: unresolved value: private__0x2d49_0x2d49 */);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
        private__0xc86_0xc86(arg2, uint256(uint160(arg1)), uint256(uint160(arg0)), 509372);
    }

    function safeTransferFrom(address arg0, address arg1, uint256 arg2, bytes calldata arg3) public {
        // block_0:
    }

    function setAdmin(address arg0) public {
        // block_0:
        private__0x11b2_0x11b2(uint256(uint160(arg0)), 562908);
    }

    function setApprovalForAll(address arg0, bool arg1) public {
        // block_0:
    }

    function setAuth(address arg0, bool arg1) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
          if (false /* TODO: evm.branch.cond18.not */) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=37
          }
          else {
            // block_2:
            emit Event_0x4D54CCAF27A8BE42C0D9691E765FA614353731AB09C493ECB4822F69CC602061(); // TODO: recover event signature
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setBaseExtension(string calldata arg0) public {
        // block_0:
    }

    function setBaseURI(string calldata arg0) public {
        // block_0:
    }

    function setDefaultRoyalty(address arg0, uint96 arg1) public {
        // block_0:
        private__0x7a0_0x7a0(arg1, uint256(uint160(arg0)), 508777);
    }

    function setIsPaused(bool arg0) public {
        // block_0:
    }

    function setMintPrice(uint256 arg0) public {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
          slot_7 = arg0;
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setTokenRoyalty(uint256 arg0, address arg1, uint96 arg2) public {
        // block_0:
        private__0x1008_0x1008(arg2, uint256(uint160(arg1)), arg0, 562686);
    }

    function supportsInterface(bytes4 arg0) public returns (bool ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x78f_0x78f */ != 0;
    }

    function symbol() public {
        // block_0:
    }

    function tokenByIndex(uint256 arg0) public returns (uint256 ret0) {
        // block_0:
        if (false /* TODO: evm.lt */) {
          // block_1:
          return 0 /* TODO: unresolved value: evm_sload */;
        }
        else {
          // block_2:
          revert(); // custom_error_candidate, selector=0xa57d13dc, args=2
        }
    }

    function tokenOfOwnerByIndex(address arg0, uint256 arg1) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xc12_0xc12 */;
    }

    function tokenURI(uint256 arg0) public {
        // block_0:
    }

    function totalNum() public returns (uint256 ret0) {
        // block_0:
        return slot_9;
    }

    function totalSupply() public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: evm_sload */;
    }

    function transferFrom(address arg0, address arg1, uint256 arg2) public {
        // block_0:
    }

    function private__0x1008_0x1008(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
          if (arg0 & (uint256(1) << uint256(96)) + uint256(type(uint256).max) < 10001) {
            // block_2:
            if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
              // block_4:
              revert(); // custom_error_candidate, selector=0x969f0852, args=2
            }
            else {
              // block_3:
            }
          }
          else {
            // block_5:
            revert(); // custom_error_candidate, selector=0xdfd1fc1b, args=3
          }
        }
        else {
          // block_6:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x1536_0x1536(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x11b2_0x11b2(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
          emit Event_0x5A272403B402D892977DF56625F4164CCAF70CA3863991C43ECFE76A6905B0A1(); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x13a6_0x13a6(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
          // block_2:
          revert(); // custom_error_candidate, selector=0x5b08ba18, args=1
        }
        else {
          // block_1:
          emit Event_0x17307EAB39AB6107E8899845AD3D59BD9653F200F220920489CA2B5937696C31((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(msg.sender)), arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
    }

    function private__0x1482_0x1482(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal {
        // block_0:
    }

    function private__0x1e0a_0x1e0a(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(2158778573) << uint256(224) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_3:
          return 1;
        }
        else {
          // block_1:
          if (uint256(1532892063) << uint256(224) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
            // block_2:
            return 1;
          }
          else {
            // block_4:
            return private__0x2609_0x2609(arg0, 724898);
          }
        }
    }

    function private__0x15f4_0x15f4(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(2014223715) << uint256(224) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_1:
          return 1;
        }
        else {
          // block_2:
          return private__0x1e0a_0x1e0a(arg0, 724416);
        }
    }

    function private__0x1619_0x1619(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (arg0 & (uint256(1) << uint256(96)) + uint256(type(uint256).max) < 10001) {
          // block_1:
          if (arg1 & (uint256(1) << uint256(160)) + uint256(type(uint256).max) == 0) {
            // block_3:
            revert(); // custom_error_candidate, selector=0xb6d9900a, args=1
          }
          else {
            // block_2:
          }
        }
        else {
          // block_4:
          revert(); // custom_error_candidate, selector=0x6f483d09, args=2
        }
    }

    function private__0x174e_0x174e(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0x1aa3_0x1aa3(uint256 arg0) internal {
        // block_0:
    }

    function private__0x1aad_0x1aad(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x2500_0x2500(arg0, arg1, 724751);
    }

    function private__0x2500_0x2500(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
    }

    function private__0x2609_0x2609(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (uint256(355110957) << uint256(225) == arg0 & uint256(0) - (uint256(1) << uint256(224))) {
          // block_1:
          return 1;
        }
        else {
          // block_2:
          return arg0 & uint256(0) - (uint256(1) << uint256(224)) == uint256(33540519) << uint256(224) ? 1 : 0;
        }
    }

    function private__0x27fb_0x27fb(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & (uint256(1) << uint256(224)) + uint256(type(uint256).max) == 0);
        // block_1:
    }

    function private__0x2ca9_0x2ca9(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x2d1c_0x2d1c(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 * arg0;
    }

    function private__0x2d49_0x2d49(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 / arg1;
    }

    function private__0x2d5d_0x2d5d(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x2e9a_0x2e9a(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg1 + arg0;
    }

    function private__0x2ead_0x2ead(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x2fea_0x2fea(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(type(uint256).max);
    }

    function private__0x78f_0x78f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return private__0x15f4_0x15f4(arg0, 722934);
    }

    function private__0x7a0_0x7a0(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
          private__0x1619_0x1619(arg0, arg1, 723011);
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0x8a0_0x8a0(uint256 arg0, uint256 arg1, uint256 arg2) internal {
        // block_0:
        private__0x174e_0x174e(uint256(uint160(msg.sender)), arg0, arg1, 723242);
    }

    function private__0xa47_0xa47(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
          if ((arg0 != 0) != (slot_0 == 0)) {
            // block_3:
            revert(); // error_string, selector=0x8c379a0, string_length=34
          }
          else {
            // block_2:
            emit Event_0x78F78EAD0839CBD5C234B44DDD6D28B52BDEC2C1016C8050BBE5557DD3EAC379(); // TODO: recover event signature
          }
        }
        else {
          // block_4:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function private__0xc86_0xc86(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }

    function private__0xfcc_0xfcc(uint256 arg0, uint256 arg1) internal {
        // block_0:
        if (uint256(uint160(msg.sender)) == (uint256(1) << uint256(160)) + uint256(type(uint256).max) & slot_0 / uint256(65536)) {
          // block_1:
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }
}
