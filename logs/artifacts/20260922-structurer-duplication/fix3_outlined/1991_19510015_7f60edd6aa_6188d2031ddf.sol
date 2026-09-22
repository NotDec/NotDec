contract Decompiled {
    event Event_0x8CA1D928F1D72493A6B78C4F74AABDE976BC37FFE2570F2A1CE5A8ABD3DDE0AA(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xAC1777479F07F3E7C34DA8402139D54027A6A260CAAAE168BDEE825CA5580DC5(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0xC052130BC4EF84580DB505783484B067EA8B71B3BCA78A7E12DB7AEA8658F004(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x3303FACD24627943A92E9DC87CFBB34B15C49B726EEC3AD3487C16BE9AB8EFE8(uint256 indexed arg0, uint256 indexed arg1, uint256 indexed arg2);
    event Event_0x4925EAFC82D0C4D67889898EEED64B18488AB19811E61620F387026DEC126A28(uint256 indexed arg0, uint256 indexed arg1);
    event Event_0x1A1CD5483E52E60B9FF7F3B9D1DB3BBD9E9D21C6324AD3A8C79DBA9B75E62F4D(uint256 indexed arg0, uint256 indexed arg1);

    mapping(uint256 => uint256) public slot_1;
    mapping(uint256 => uint256) public slot_2;
    uint256[] public slot_3;
    uint256[] public slot_4;
    uint256 public slot_5;
    mapping(uint256 => uint256) public slot_57;
    mapping(uint256 => uint256) public slot_58;
    mapping(uint256 => uint256) public slot_59;
    uint256 public slot_6;

    function EMISSION_MANAGER() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893);
    }

    function REVISION() public returns (uint256 ret0) {
        // block_0:
        return 1;
    }

    function public_0x955c2ad7() public {
        // block_0:
    }

    function claimAllRewardsOnBehalf(address arg0, address arg1, address arg2) public {
        // block_0:
    }

    function claimAllRewardsToSelf(address arg0) public {
        // block_0:
    }

    function claimAllRewards(address arg0, address arg1) public {
        // block_0:
    }

    function claimRewardsOnBehalf(address arg0, uint256 arg1, address arg2, address arg3, address arg4) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x654_0x654 */;
    }

    function claimRewardsToSelf(address arg0, uint256 arg1, address arg2) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xa6c_0xa6c */;
    }

    function claimRewards(address arg0, uint256 arg1, address arg2, address arg3) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0x5f9_0x5f9 */;
    }

    function fallback() public {
        // block_0:
        revert(); // empty
    }

    function getAllUserRewards(address arg0, address arg1) public {
        // block_0:
    }

    function getAssetDecimals(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] / (uint256(1) << uint256(128)) & uint256(255);
    }

    function getAssetIndex(address arg0, address arg1) public returns (uint256 ret0, uint256 ret1) {
        // block_0:
        return (0 /* TODO: unresolved value: private.ret4 */, 0 /* TODO: unresolved value: private.ret3 */);
    }

    function getClaimer(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_57[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function getDistributionEnd(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return private__0x5bf_0x5bf(uint256(uint160(arg1)), uint256(uint160(arg0)), 412500);
    }

    function getEmissionManager() public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893);
    }

    function getRewardOracle(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_59[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function getRewardsByAsset(address arg0) public {
        // block_0:
    }

    function getRewardsData(address arg0, address arg1) public returns (uint256 ret0, uint256 ret1, uint256 ret2, uint256 ret3) {
        // block_0:
        return (slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] & (uint256(1) << uint256(104)) + uint256(type(uint256).max), slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] / (uint256(1) << uint256(104)) & (uint256(1) << uint256(88)) + uint256(type(uint256).max), slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] / (uint256(1) << uint256(192)) & uint256(4294967295), slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] / (uint256(1) << uint256(224)) & uint256(4294967295));
    }

    function getRewardsList() public {
        // block_0:
    }

    function getTransferStrategy(address arg0) public returns (uint256 ret0) {
        // block_0:
        return slot_58[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0))] & (uint256(1) << uint256(160)) + uint256(type(uint256).max) & (uint256(1) << uint256(160)) + uint256(type(uint256).max);
    }

    function getUserAccruedRewards(address arg0, address arg1) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xf06_0xf06 */;
    }

    function getUserAssetIndex(address arg0, address arg1, address arg2) public returns (uint256 ret0) {
        // block_0:
        return (uint256(1) << uint256(104)) + uint256(type(uint256).max) & slot_1[uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)];
    }

    function getUserRewards(address arg0, address arg1, address arg2) public returns (uint256 ret0) {
        // block_0:
        return 0 /* TODO: unresolved value: private__0xb8c_0xb8c */;
    }

    function handleAction(address arg0, uint256 arg1, uint256 arg2) public {
        // block_0:
        require(msg.data.length > 99);
        // block_1:
        private__0x2edf_0x2edf(uint256(uint160(arg0)), 12328);
        private__0x643_0x643(arg2, arg1, uint256(uint160(arg0)), 547651);
    }

    function initialize(address arg0) public {
        // block_0:
    }

    function setClaimer(address arg0, address arg1) public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893) == uint256(uint160(msg.sender))) {
          // block_1:
          emit Event_0x4925EAFC82D0C4D67889898EEED64B18488AB19811E61620F387026DEC126A28((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setDistributionEnd(address arg0, address arg1, uint32 arg2) public {
        // block_0:
        if ((uint256(1) << uint256(160)) + uint256(1461501637330902918203684832716283019655932542975) & uint256(1443110500457411796578838204131955442359735615893) == uint256(uint160(msg.sender))) {
          // block_1:
          emit Event_0xAC1777479F07F3E7C34DA8402139D54027A6A260CAAAE168BDEE825CA5580DC5((uint256(1) << uint256(160)) + uint256(type(uint256).max) & uint256(uint160(arg0)), uint256(uint160(arg1)) & (uint256(1) << uint256(160)) + uint256(type(uint256).max)); // TODO: recover event signature
        }
        else {
          // block_2:
          revert(); // error_string, selector=0x8c379a0
        }
    }

    function setEmissionPerSecond(address arg0, address arg1, uint88 arg2) public {
        // block_0:
    }

    function setRewardOracle(address arg0, address arg1) public {
        // block_0:
    }

    function setTransferStrategy(address arg0, address arg1) public {
        // block_0:
    }

    function private__0x2c17_0x2c17(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        if (arg0 > (uint256(1) << uint256(128)) + uint256(type(uint256).max)) {
          // block_2:
          revert(); // error_string, selector=0x8c379a0, string_length=39
        }
        else {
          // block_1:
          return arg0;
        }
    }

    function private__0x2e56_0x2e56(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3, uint256 arg4) internal returns (uint256 ret0) {
        // block_0:
        return private__0x37e3_0x37e3(arg3, private__0x37a8_0x37a8(arg2, arg1, 11875), 11885) / arg0;
    }

    function private__0x2edf_0x2edf(uint256 arg0, uint256 arg1) internal {
        // block_0:
        require(arg0 & uint256(0) - (uint256(1) << uint256(160)) == 0);
        // block_1:
    }

    function private__0x3594_0x3594(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x35ec_0x35ec(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + arg1;
    }

    function private__0x3604_0x3604(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(1);
    }

    function private__0x361f_0x361f(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return arg0 + uint256(96);
    }

    function private__0x364e_0x364e(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) {
        // block_0:
        return (arg0 & (uint256(1) << uint256(128)) + uint256(type(uint256).max)) + uint256(1);
    }

    function private__0x37a8_0x37a8(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 - arg1;
    }

    function private__0x37e3_0x37e3(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return arg0 * arg1;
    }

    function private__0x5bf_0x5bf(uint256 arg0, uint256 arg1, uint256 arg2) internal returns (uint256 ret0) {
        // block_0:
        return slot_1[(uint256(1) << uint256(160)) + uint256(type(uint256).max) & arg1] / (uint256(1) << uint256(224)) & uint256(4294967295);
    }

    function private__0x643_0x643(uint256 arg0, uint256 arg1, uint256 arg2, uint256 arg3) internal {
        // block_0:
    }
}
