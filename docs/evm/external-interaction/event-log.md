# Event log

## 简介

Solidity event 会写入交易日志：

```solidity
contract C {
    event Transfer(address indexed from, address indexed to, uint256 amount);

    function emitTransfer(address from, address to, uint256 amount) external {
        emit Transfer(from, to, amount);
    }
}
```

event 不会写 storage，也不会影响合约后续执行。它会生成 log，供链下程序读取。

EVM 用 `LOG0` 到 `LOG4` 指令写日志。数字表示 topic 数量。

event log 是交易执行结果的一部分，保存在 transaction receipt 的 `logs` 里，不在合约
storage 里。链上合约后续不能读取历史 log；如果某个数据以后还要被链上逻辑使用，必须
写进 storage。

如果交易最终 revert，这次交易里已经写出的 log 也会被回滚，不会出现在成功 receipt 里。

## 编译过程与低层模式

event 编码分两部分：

- topic：最多 4 个，每个 32 字节。
- data：一段 memory buffer。

非 anonymous event 的 `topic0` 是事件签名 hash：

```text
keccak256("Transfer(address,address,uint256)")
```

`indexed` 参数放进 topic。非 indexed 参数放进 data buffer。

Solidity 普通 event 最多只能有 3 个 `indexed` 参数，因为 `topic0` 要留给事件签名 hash。
anonymous event 没有事件签名 topic，所以最多可以有 4 个 `indexed` 参数。

上面的例子里：

```solidity
event Transfer(address indexed from, address indexed to, uint256 amount);
```

topic 是：

```text
topic0 = event signature hash
topic1 = from
topic2 = to
```

data 是：

```text
amount
```

简化 EVM 伪指令：

```text
ptr = mload(0x40)
mstore(ptr, amount)
LOG3(ptr, 32, topic0, from, to)
```

如果 event 是 `anonymous`，就没有 topic0，indexed 参数从 topic0 开始放。

动态 indexed 参数比较特殊。比如：

```solidity
event E(string indexed name);
```

`name` 不会直接塞进 topic，而是先对它的 ABI 编码取 hash：

```text
topic = keccak256(encoded_name)
```

非 indexed 动态参数则放在 data buffer 里，按 ABI head/tail 编码。

## 例子详解

Solidity 源码：

```solidity
contract C {
    event Set(address indexed user, uint256 oldValue, uint256 newValue);

    function set(uint256 oldValue, uint256 newValue) external {
        emit Set(msg.sender, oldValue, newValue);
    }
}
```

编译器要保证的语义是：

- 日志里能看出这是 `Set(address,uint256,uint256)` 事件。
- `user` 是 indexed 参数，所以放 topic。
- `oldValue` 和 `newValue` 不是 indexed，所以放 data buffer。

简化 EVM 伪指令：

```text
topic0 = keccak256("Set(address,uint256,uint256)")
topic1 = CALLER

ptr = mload(0x40)
mstore(ptr + 0, oldValue)
mstore(ptr + 32, newValue)

LOG2(ptr, 64, topic0, topic1)
```

再看 anonymous event：

```solidity
event Seen(address indexed user) anonymous;
```

它没有事件签名 topic：

```text
topic0 = user
LOG1(0, 0, topic0)
```

动态非 indexed 参数例子：

```solidity
event Message(address indexed user, string text);
```

简化 data buffer：

```text
topic0 = keccak256("Message(address,string)")
topic1 = user

ptr = mload(0x40)
mstore(ptr + 0, 32)       ; text tail offset
mstore(ptr + 32, len)
mstore(ptr + 64, bytes)

LOG2(ptr, total_size, topic0, topic1)
```

需要记住的是：event 的 topic 和 data 分工很固定。indexed 参数进 topic，非 indexed 参数进
memory data buffer；非 anonymous event 还会多一个事件签名 topic。

## 链下程序如何使用

链下程序一般通过 RPC 节点读取 receipt logs。常见方式有两种：

1. 查历史日志，比如用 `eth_getLogs` 按合约地址、区块范围和 topic 过滤。
2. 实时监听日志，比如钱包、前端、indexer 或后端服务订阅某个 event。

例如 ERC20 `Transfer(address,address,uint256)`，链下程序可以用事件签名 hash 过滤
`topic0`，再用 `from`、`to` 的 indexed topic 过滤地址。拿到 log 后，再按 ABI 解码 data。

这种交互模式通常是：

```text
用户发交易
合约执行并 emit event
EVM 执行 LOGn
节点把 log 放进 receipt
链下程序读取或监听 receipt logs
链下程序更新 UI、数据库、索引或触发后续任务
```

event 适合记录“发生过什么”，比如转账、订单成交、mint、提款、管理员改参数等。它比把
历史记录全写进 storage 更便宜，也更适合链下按区块顺序重放。

但链下程序不能无条件相信 event。合约可以 emit 一个看起来像转账的事件，但实际没有改
余额。关键逻辑最好结合合约状态、标准接口或业务规则再确认。

链下监听还要处理区块重组。对重要业务，通常等几个 confirmation 后再把事件当成最终结果。

## 从字节码能恢复什么

EVM 只认识 `LOG0` 到 `LOG4`，不认识 Solidity event 名字。普通 event 的名字和参数类型会先
拼成签名字符串，再取 hash 放进 `topic0`：

```text
keccak256("Transfer(address,address,uint256)")
```

所以仅凭字节码通常只能看到这个 32 字节 hash，看不到 `Transfer` 这个名字，也看不到
`from`、`to`、`amount` 这些参数名。hash 不可逆，反编译器不能从 hash 本身可靠还原原始
事件签名。

实际恢复 event 名字通常依赖：

- ABI 或源码。
- 合约 metadata。
- 已知签名库，比如常见 ERC20 / ERC721 事件签名。
- 人工命名。

反编译器仍然可以从 `LOGn` 附近恢复一些低层信息：

- 有几个 topic。
- 哪个 topic 是常量事件签名 hash。
- data buffer 的 offset 和 size。
- indexed 参数来自调用者、函数参数、storage 读取还是其他表达式。
- data 里大概有几个 32 字节字段，或者是否有动态 ABI head/tail 编码。

anonymous event 没有事件签名 topic，所以仅凭字节码更难知道它对应源码里的哪个 event。
