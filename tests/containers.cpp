#include "sorted_set.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <type_traits>

#define CHECK(expr) do { if (!(expr)) { std::cerr << "Check failed at " << __LINE__ << ": " << #expr << '\n'; std::abort(); } } while (false)
template<class Exception, class F> void throws(F f) {
    bool caught = false;
    try { f(); } catch (const Exception&) { caught = true; }
    CHECK(caught);
}
void neighbor(const int* p, const std::vector<int>& v, std::size_t i) {
    if (i >= v.size()) CHECK(p == nullptr);
    else { CHECK(p != nullptr); CHECK(*p == v[i]); }
}
template<bool Multi> void randomized() {
    sorted_set::SortedCollection<int, Multi> s;
    std::conditional_t<Multi, std::multiset<int>, std::set<int>> ref;
    std::mt19937 rng(812);
    for (int step = 0; step < 15000; ++step) {
        const int x = static_cast<int>(rng() % 401) - 200;
        const auto op = rng() % 5;
        if (op < 3) {
            const bool added = Multi || ref.find(x) == ref.end();
            CHECK(s.add(x) == added); ref.insert(x);
        } else if (op == 3) {
            const auto it = ref.find(x);
            CHECK(s.discard(x) == (it != ref.end()));
            if (it != ref.end()) ref.erase(it);
        } else if (!ref.empty()) {
            const auto i = static_cast<std::ptrdiff_t>(rng() % ref.size());
            auto it = std::next(ref.begin(), i);
            const auto signed_i = rng() % 2 ? i : i - static_cast<std::ptrdiff_t>(ref.size());
            CHECK(s[signed_i] == *it); CHECK(s.pop(signed_i) == *it); ref.erase(it);
        } else { throws<std::out_of_range>([&] { s.pop(); }); }
        std::vector<int> v(ref.begin(), ref.end());
        CHECK(s.size() == v.size());
        CHECK(std::vector<int>(s.begin(), s.end()) == v);
        CHECK(std::vector<int>(s.rbegin(), s.rend()) == std::vector<int>(v.rbegin(), v.rend()));
        for (const auto& a : s.buckets()) CHECK(!a.empty());
        auto l = static_cast<std::size_t>(std::lower_bound(v.begin(), v.end(), x) - v.begin());
        auto r = static_cast<std::size_t>(std::upper_bound(v.begin(), v.end(), x) - v.begin());
        CHECK(s.index(x) == l); CHECK(s.index_right(x) == r);
        CHECK(s.count(x) == r - l); CHECK(s.contains(x) == (l != r));
        neighbor(s.lt(x), v, l ? l - 1 : v.size());
        neighbor(s.le(x), v, r ? r - 1 : v.size());
        neighbor(s.ge(x), v, l); neighbor(s.gt(x), v, r);
        throws<std::out_of_range>([&] { s.at(static_cast<std::ptrdiff_t>(s.size())); });
        throws<std::out_of_range>([&] { s.at(-static_cast<std::ptrdiff_t>(s.size()) - 1); });
        throws<std::out_of_range>([&] { s.at(std::numeric_limits<std::ptrdiff_t>::min()); });
    }
    while (!ref.empty()) { CHECK(s.pop(0) == *ref.begin()); ref.erase(ref.begin()); }
    CHECK(s.buckets().empty()); CHECK(s.add(7)); s.clear(); CHECK(s.empty());
}
void lists() {
    sorted_set::BucketList<int> s;
    std::vector<int> v;
    for (int i = 0; i < 1000; ++i) { s.append(i); v.push_back(i); }
    std::mt19937 rng(921);
    for (int step = 0; step < 10000; ++step) {
        const int x = static_cast<int>(rng() % 20);
        switch (rng() % 6) {
        case 0: s.append(x); v.push_back(x); break;
        case 1: {
            auto i = static_cast<std::ptrdiff_t>(rng() % (v.size() + 1));
            s.insert(i, x); v.insert(v.begin() + i, x); break;
        }
        case 2: if (!v.empty()) {
            auto i = static_cast<std::ptrdiff_t>(rng() % v.size());
            CHECK(s.pop(i - static_cast<std::ptrdiff_t>(v.size())) == v[i]); v.erase(v.begin() + i);
        } break;
        case 3: s.reverse(); std::reverse(v.begin(), v.end()); break;
        case 4: {
            auto it = std::find(v.begin(), v.end(), x);
            if (it == v.end()) throws<std::invalid_argument>([&] { s.remove(x); });
            else { CHECK(s.index(x) == static_cast<std::size_t>(it - v.begin())); s.remove(x); v.erase(it); }
            break;
        }
        default: if (!v.empty()) { s[-1] = x; v.back() = x; }
        }
        CHECK(s.size() == v.size()); CHECK(std::vector<int>(s.begin(), s.end()) == v);
        CHECK(s.count(x) == static_cast<std::size_t>(std::count(v.begin(), v.end(), x)));
        CHECK(s.contains(x) == (std::find(v.begin(), v.end(), x) != v.end()));
        throws<std::out_of_range>([&] { s.insert(static_cast<std::ptrdiff_t>(v.size()) + 1, x); });
    }
    CHECK(s == s.copy()); s.clear();
    throws<std::out_of_range>([&] { s.insert(-2, 7); });
    s.insert(-1, 7); s.insert(-1, 8); CHECK(s.pop() == 7); CHECK(s.pop() == 8);
}
int main() {
    randomized<false>(); randomized<true>(); lists();
    sorted_set::SortedSet<int> a{3, 1, 3, 2}, b{1, 2, 3}; CHECK(a == b);
    a.clear(); for (int i = 0; i < 10000; ++i) a.add(i);
    for (int i = 0; i < 10000; ++i) CHECK(a.discard(i));
    sorted_set::SortedMultiset<int> m(std::vector<int>(5000, 4));
    CHECK(m.count(4) == 5000); CHECK(!m.lt(4)); CHECK(!m.gt(4));
    for (int i = 0; i < 5000; ++i) CHECK(m.discard(4));
    sorted_set::SortedSet<int, std::greater<int>> desc{1, 2, 3};
    CHECK(desc[0] == 3); CHECK(*desc.lt(2) == 3); CHECK(*desc.gt(2) == 1);
    CHECK(*desc.le(2) == 2); CHECK(*desc.ge(2) == 2); CHECK(desc.index(2) == 1);
    CHECK(desc.add(4)); CHECK(desc.discard(2)); CHECK(desc.index_right(3) == 2);
    sorted_set::SortedSet<std::string> strings{"z", "a", "a"}; CHECK(strings.size() == 2);
    struct Deref { bool operator()(const std::unique_ptr<int>& x, const std::unique_ptr<int>& y) const { return *x < *y; } };
    sorted_set::SortedSet<std::unique_ptr<int>, Deref> move_only;
    move_only.add(std::make_unique<int>(2)); move_only.add(std::make_unique<int>(1));
    CHECK(*move_only.pop() == 2);
    auto moved = std::move(move_only); CHECK(move_only.empty()); CHECK(*moved[0] == 1);
    move_only.add(std::make_unique<int>(9)); CHECK(*move_only[0] == 9);
    static_assert(std::is_same_v<decltype(*b.begin()), const int&>);
    std::cout << "All container tests passed\n";
}
