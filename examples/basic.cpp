#include "sorted_set.hpp"
#include <iostream>

int main() {
    sorted_set::SortedSet<int> set{3, 1, 4, 1, 5};
    set.add(2);
    std::cout << "rank(3) = " << set.index(3) << '\n';
    if (const auto* x = set.lt(3)) std::cout << "lt(3) = " << *x << '\n';
    std::cout << "max = " << set[-1] << '\n';
    for (int x : set) std::cout << x << ' ';
    std::cout << '\n';

    sorted_set::SortedMultiset<int> multi{2, 2, 1};
    multi.discard(2); // Removes one occurrence.
    std::cout << "count(2) = " << multi.count(2) << '\n';

    sorted_set::BucketList<int> list{3, 1, 2};
    list.insert(-1, 9);
    list.reverse();
    for (int x : list) std::cout << x << ' ';
    std::cout << '\n';
}
