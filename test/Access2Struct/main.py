from typing import List


class Type:
    pass

class StructType(Type):
    def __init__(self):
        self.members = []  # offset, size, type

    def __str__(self):
        return 'StructType(%s)' % ', '.join('%d-%s: %s' % (offset, offset+size, type) for offset, size, type in self.members)

class UnionType(Type):
    def __init__(self):
        self.members = []  # offset, size, type

    def __str__(self):
        return 'UnionType(%s)' % ', '.join(str(type) for type in self.members)

class ArrayType(Type):
    def __init__(self, element_type, stride):
        self.element_type = element_type
        self.stride = stride

    def __str__(self):
        return 'ArrayType(%s, %d)' % (self.element_type, self.stride)

class BasicType(Type):
    def __init__(self, size):
        self.size = size

    def __str__(self):
        return 'BasicType(%d)' % self.size


def create_struct(members):
    # 如果仅有一个off=0的成员，则为简单的内部类型
    if (len(members) == 1) and (0 == members[0][0]):
        return members[0][2]
    r = StructType()
    r.members = members
    return r
    
def create_union(members):
    # 根据结束偏移贪心
    members = sorted(members, key=lambda x:x[0] + x[1])
    union_panels = [] # type: List[List]
    for mb in members:
        for panel in union_panels:
            if panel[-1][0] + panel[-1][1] <= mb[0]:
                panel.append(mb)
                break
        else:
            union_panels.append([mb])    

    # 仅有一个union成员，则是普通结构体
    if len(union_panels) == 1:
        return create_struct(union_panels[0])
    else:
        r = UnionType()
        for panel in union_panels:
            r.members.append(create_struct(panel))
        return r


def infer_type(patterns):
    if not patterns:
        return StructType()  # 默认为空结构体

    members = []

    if len(patterns) == 1 and patterns[0]['base'] == 0 and len(patterns[0]['strides']) == 0:
        # 简单结构体类型
        return BasicType(size=patterns[0]['access_size'])

    # 提取所有乘数项
    all_strides = set()
    for pattern in patterns:
        all_strides.update(pattern['strides'])

    remaining_patterns = patterns

    while len(all_strides) > 0:
        # 处理乘数项的最大值所代表的数组。
        max_stride = max(all_strides)
        all_strides.remove(max_stride)

        # 提取包含max_stride的模式
        has_stride_patterns = []
        for pattern in remaining_patterns:
            if max_stride in pattern['strides']:
                has_stride_patterns.append(pattern)
        for pattern in has_stride_patterns:
            remaining_patterns.remove(pattern)

        # 按基址排序
        has_stride_patterns = list(sorted(has_stride_patterns, key=lambda x: x['base']))
        
        while len(has_stride_patterns) > 0:
            range_start = has_stride_patterns[0]['base']
            range_end = has_stride_patterns[0]['base'] + max_stride

            # 提取当前组中Base在当前范围内的模式
            in_range_patterns = []
            for pattern in has_stride_patterns:
                if pattern['base'] >= range_start and pattern['base'] < range_end:
                    in_range_patterns.append(pattern)
            for pattern in in_range_patterns:
                has_stride_patterns.remove(pattern)

            sub_array_patterns = []
            # 转换为子问题的访问模式
            for pattern in in_range_patterns:
                new_strides = pattern['strides'].copy()
                new_strides.remove(max_stride)
                new_base = pattern['base'] - range_start

                # 递归处理剩余乘数项
                sub_pattern = {
                    'base': new_base,
                    'strides': new_strides,
                    'access_size': pattern['access_size']
                }
                sub_array_patterns.append(sub_pattern)
            member_type = ArrayType(infer_type(sub_array_patterns), max_stride)
            # 添加成员到结构体
            members.append((range_start, max_stride, member_type))

    if len(remaining_patterns) > 0:
        # 处理剩余的基本类型
        for pattern in remaining_patterns:
            assert len(pattern['strides']) == 0
            members.append((pattern['base'], pattern['access_size'], BasicType(pattern['access_size'])))

    def is_overlap(range1, range2):
        s1, e1 = range1
        s2, e2 = range2
        assert(s1 <= e1)
        assert(s2 <= e2)
        return max(s1, s2) < min(e1, e2)

    def filter_in_range(members, start, end):
        in_range_members = []
        for mb in members:
            if is_overlap((start, end), (mb[0], mb[0]+mb[1])):
                in_range_members.append(mb)
        return in_range_members

    # 根据成员创建结构体和union类型
    # member: (start, size, type)

    while True:
        # 收集所有分割点
        all_index = set()
        for mb in members:
            all_index.add(mb[0])
            all_index.add(mb[0] + mb[1])
        all_index = list(sorted(list(all_index)))
        
        # 遍历所有最小范围区间，如果出现重叠则以此开始创建union类型。
        for i in range(len(all_index) - 1):
            start = all_index[i]
            end = all_index[i+1]
            in_range_members = filter_in_range(members, start, end)
            if len(in_range_members) <= 1:
                continue
            # try to create union
            old_len = 1
            new_len = len(in_range_members)
            while new_len > old_len:
                old_len = new_len
                start = min([start for start, size, ty in in_range_members])
                end = max([start+size for start, size, ty in in_range_members])
                in_range_members = filter_in_range(members, start, end)
                new_len = len(in_range_members)
            # replace member in in_range_members to a union member.
            non_overlap_members = []
            union_start = start
            union_end = end
            overlap_members = []
            for mb in members:
                if is_overlap((start, end), (mb[0], mb[0]+mb[1])):
                    overlap_members.append((mb[0] - union_start, mb[1], mb[2]))
                else:
                    non_overlap_members.append(mb)
            non_overlap_members.append((union_start, union_end - union_start, create_union(overlap_members)))
            members = non_overlap_members
        else:
            break

    # 返回结构体类型
    return create_struct(members)

import unittest

class TestAccess(unittest.TestCase):
    def test_1_Basic(self):
        patterns = [
            {'base': 0, 'strides': set(), 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)
    
    def test_2_Basic(self):
        patterns = [
            {'base': 0, 'strides': {4}, 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)
    
    def test_3(self):
        patterns = [
            {'base': 0, 'strides': {4}, 'access_size': 4},
            {'base': 4, 'strides': {4}, 'access_size': 4},
            {'base': 8, 'strides': {4}, 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)
    
    def test_4(self):
        patterns = [
            {'base': 0, 'strides': {84}, 'access_size': 4},
            {'base': 4, 'strides': {8,84}, 'access_size': 4},
            {'base': 8, 'strides': {8,84}, 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)
    
    def test_5(self):
        patterns = [
            {'base': 0, 'strides': set(), 'access_size': 4},
            {'base': 4, 'strides': set(), 'access_size': 4},
            {'base': 8, 'strides': set(), 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)

    def test_6_Merge(self):
        patterns = [
            {'base': 0, 'strides': set(), 'access_size': 4},
            {'base': 0, 'strides': {4}, 'access_size': 4},
        ]
        type = infer_type(patterns)
        # self.assertEqual(str(type), 'ArrayType(StructType(0: BasicType(4)<4>, 4: BasicType(4)<4>, 8: BasicType(4)<4>, 12: BasicType(4)<4>), 8)')
        print(type)

if __name__ == '__main__':
    unittest.main()
