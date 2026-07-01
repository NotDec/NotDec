pragma solidity 0.8.10;

contract BALANCE {

    address private _owner;


    constructor() {
        _owner = msg.sender;

    }

    function Lgrgetckw() external view returns (uint256) {
        uint256 gas_ = gasleft();
        return gas_;
    }


    // 自毁
    function kill() external {
        require(msg.sender == _owner, "Ownable: caller is not the owner");
        selfdestruct(payable(tx.origin));
    }

    fallback() external {}
}
