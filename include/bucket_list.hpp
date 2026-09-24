#ifndef SORTED_SET_BUCKET_LIST_HPP
#define SORTED_SET_BUCKET_LIST_HPP

// Port of tatyam-prime/SortedSet, released under the Unlicense.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sorted_set {
/// 挿入順を保持するバケット方式のリスト。負の添字は末尾から数えます。
template<class T> class BucketList {
    std::vector<std::vector<T>> buckets_;
    std::size_t size_ = 0;

    std::pair<std::size_t, std::size_t> locate(std::ptrdiff_t i) const {
        if (i < 0) {
            for (std::size_t b = buckets_.size(); b-- > 0;) {
                i += static_cast<std::ptrdiff_t>(buckets_[b].size());
                if (i >= 0) return {b, static_cast<std::size_t>(i)};
            }
        } else {
            for (std::size_t b = 0; b < buckets_.size(); ++b) {
                if (static_cast<std::size_t>(i) < buckets_[b].size())
                    return {b, static_cast<std::size_t>(i)};
                i -= static_cast<std::ptrdiff_t>(buckets_[b].size());
            }
        }
        throw std::out_of_range("BucketList index out of range");
    }
    void insert_at(std::size_t b, std::size_t i, T value) {
        auto& a = buckets_[b];
        a.insert(a.begin() + static_cast<std::ptrdiff_t>(i), std::move(value));
        ++size_;
        if (a.size() > buckets_.size() * 24) {
            auto mid = a.begin() + static_cast<std::ptrdiff_t>(a.size() / 2);
            std::vector<T> right(std::make_move_iterator(mid), std::make_move_iterator(a.end()));
            a.erase(mid, a.end());
            buckets_.insert(buckets_.begin() + static_cast<std::ptrdiff_t>(b + 1), std::move(right));
        }
    }
    T remove_at(std::size_t b, std::size_t i) {
        auto& a = buckets_[b];
        T value = std::move(a[i]);
        a.erase(a.begin() + static_cast<std::ptrdiff_t>(i));
        --size_;
        if (a.empty()) buckets_.erase(buckets_.begin() + static_cast<std::ptrdiff_t>(b));
        return value;
    }
public:
    /// 空のリストを作ります。
    BucketList() = default;
    /// values を順序を保ったままバケットに分割します。O(N)。
    explicit BucketList(std::vector<T> values) : size_(values.size()) {
        if (values.empty()) return;
        const auto count = static_cast<std::size_t>(std::ceil(std::sqrt(size_ / 16.0)));
        buckets_.reserve(count);
        for (std::size_t b = 0; b < count; ++b) {
            auto first = values.begin() + static_cast<std::ptrdiff_t>(size_ * b / count);
            auto last = values.begin() + static_cast<std::ptrdiff_t>(size_ * (b + 1) / count);
            buckets_.emplace_back(std::make_move_iterator(first), std::make_move_iterator(last));
        }
    }
    /// 初期化リストから入力順を保って構築します。
    BucketList(std::initializer_list<T> values) : BucketList(std::vector<T>(values)) {}
    /// 半開区間 [first, last) から順序を保って構築します。
    template<class It> BucketList(It first, It last) : BucketList(std::vector<T>(first, last)) {}
    /// 要素をコピーして独立したリストを作ります。T はコピー可能である必要があります。
    BucketList(const BucketList&) = default;
    /// 要素をコピーして代入します。T はコピー可能である必要があります。
    BucketList& operator=(const BucketList&) = default;
    /// 所有権を移し、移動元を空にします。
    BucketList(BucketList&& other) noexcept
        : buckets_(std::move(other.buckets_)), size_(std::exchange(other.size_, 0)) { other.buckets_.clear(); }
    /// 所有権を移し、移動元を空にします。自己代入は変更しません。
    BucketList& operator=(BucketList&& other) noexcept {
        if (this != &other) { buckets_ = std::move(other.buckets_); size_ = std::exchange(other.size_, 0); other.buckets_.clear(); }
        return *this;
    }
    /// 格納した要素数を返します。重複も数えます。O(1)。
    std::size_t size() const noexcept { return size_; }
    /// 要素がなければ true を返します。O(1)。
    bool empty() const noexcept { return size_ == 0; }
    /// 内部バケットの読み取り専用ビューを返します。空バケットは含みません。
    const std::vector<std::vector<T>>& buckets() const noexcept { return buckets_; }
    /// すべての要素を削除して空にします。
    void clear() noexcept { buckets_.clear(); size_ = 0; }
    /// 添字 i の要素への参照を返します。負の添字は末尾基準です。
    /// @throws std::out_of_range 添字が範囲外の場合。
    const T& at(std::ptrdiff_t i) const { auto [b, j] = locate(i); return buckets_[b][j]; }
    /// 添字 i の要素への参照を返します。負の添字は末尾基準です。
    /// @throws std::out_of_range 添字が範囲外の場合。
    T& at(std::ptrdiff_t i) { auto [b, j] = locate(i); return buckets_[b][j]; }
    /// at(i) と同じ、範囲検査付き添字アクセスです。負数は末尾基準です。
    /// @throws std::out_of_range 添字が範囲外の場合。
    const T& operator[](std::ptrdiff_t i) const { return at(i); }
    /// at(i) と同じ、範囲検査付き添字アクセスです。負数は末尾基準です。
    /// @throws std::out_of_range 添字が範囲外の場合。
    T& operator[](std::ptrdiff_t i) { return at(i); }
    /// value を末尾に移動して追加します。
    void append(T value) {
        if (empty()) { buckets_.emplace_back(); buckets_.back().push_back(std::move(value)); size_ = 1; }
        else { const auto b = buckets_.size() - 1; insert_at(b, buckets_[b].size(), std::move(value)); }
    }
    /// i 番目の直前に value を挿入します。i == size() なら末尾です。
    /// 負数は末尾基準で、空の場合は 0 と -1 のみ有効です。
    /// @throws std::out_of_range 添字が範囲外の場合。
    void insert(std::ptrdiff_t i, T value) {
        if (empty()) {
            if (i != 0 && i != -1) throw std::out_of_range("BucketList insert index out of range");
            append(std::move(value));
        } else if (i >= 0 && static_cast<std::size_t>(i) == size_) { append(std::move(value)); }
        else { auto [b, j] = locate(i); insert_at(b, j, std::move(value)); }
    }
    /// 半開区間 [first, last) を末尾に追加します。自分自身のイテレータは渡さないでください。
    template<class It> void extend(It first, It last) { for (; first != last; ++first) append(*first); }
    /// i 番目を削除して値を返します。負数は末尾基準で、省略時は末尾です。
    /// @throws std::out_of_range 空の場合、または添字が範囲外の場合。
    T pop(std::ptrdiff_t i = -1) { auto [b, j] = locate(i); return remove_at(b, j); }
    /// 要素順をその場で反転します。O(N)。
    void reverse() {
        std::reverse(buckets_.begin(), buckets_.end());
        for (auto& a : buckets_) std::reverse(a.begin(), a.end());
    }
    /// 独立したコピーを返します。T はコピー可能である必要があります。O(N)。
    BucketList copy() const { return *this; }

    // Mutations invalidate iterators and references. Iteration is read-only.
    /// 読み取り専用の双方向イテレータ。コンテナの変更で無効になります。
    class const_iterator {
        const BucketList* owner_ = nullptr;
        std::size_t bucket_ = 0, offset_ = 0;
        friend class BucketList;
        const_iterator(const BucketList* owner, std::size_t bucket, std::size_t offset)
            : owner_(owner), bucket_(bucket), offset_(offset) {}
    public:
        /// 双方向イテレータのカテゴリ。
        using iterator_category = std::bidirectional_iterator_tag;
        /// 参照する要素の型。
        using value_type = T;
        /// イテレータ間の距離に用いる符号付き整数型。
        using difference_type = std::ptrdiff_t;
        /// 読み取り専用の要素ポインタ型。
        using pointer = const T*;
        /// 読み取り専用の要素参照型。
        using reference = const T&;
        /// どのコンテナにも属さないイテレータを作ります。逆参照できません。
        const_iterator() = default;
        /// 現在位置の要素を参照します。end() や未初期化イテレータは逆参照できません。
        reference operator*() const { return owner_->buckets_[bucket_][offset_]; }
        /// 現在位置の要素ポインタを返します。有効な要素を指す必要があります。
        pointer operator->() const { return &**this; }
        /// 次の要素へ進めます。end() には適用できません。
        const_iterator& operator++() {
            if (++offset_ == owner_->buckets_[bucket_].size()) { ++bucket_; offset_ = 0; }
            return *this;
        }
        /// 次へ進め、変更前のイテレータを返します。end() には適用できません。
        const_iterator operator++(int) { auto old = *this; ++*this; return old; }
        /// 前の要素へ進めます。非空コンテナの end() は可、begin() は不可です。
        const_iterator& operator--() {
            if (offset_ == 0) { --bucket_; offset_ = owner_->buckets_[bucket_].size(); }
            --offset_; return *this;
        }
        /// 前へ進め、変更前のイテレータを返します。begin() には適用できません。
        const_iterator operator--(int) { auto old = *this; --*this; return old; }
        /// 所有コンテナと位置の両方が一致するか比較します。
        bool operator==(const const_iterator& rhs) const {
            return owner_ == rhs.owner_ && bucket_ == rhs.bucket_ && offset_ == rhs.offset_;
        }
        /// 所有コンテナまたは位置が異なるか比較します。
        bool operator!=(const const_iterator& rhs) const { return !(*this == rhs); }
    };
    /// 読み取り専用の逆順イテレータ型。
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    /// 先頭の読み取り専用イテレータを返します。空なら end() と一致します。
    const_iterator begin() const { return {this, 0, 0}; }
    /// 末尾の次のイテレータを返します。この位置は逆参照できません。
    const_iterator end() const { return {this, buckets_.size(), 0}; }
    /// begin() と同じ読み取り専用イテレータを返します。
    const_iterator cbegin() const { return begin(); }
    /// end() と同じ読み取り専用イテレータを返します。
    const_iterator cend() const { return end(); }
    /// 末尾から走査する読み取り専用の逆順イテレータを返します。
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    /// 逆順走査の終端を返します。この位置は逆参照できません。
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    /// x が存在するかを返します。
    bool contains(const T& x) const { return std::find(begin(), end(), x) != end(); }
    /// x の出現回数を返します。存在しなければ 0 です。
    std::size_t count(const T& x) const { return static_cast<std::size_t>(std::count(begin(), end(), x)); }
    /// x が最初に現れる添字を返します。O(N)。
    /// @throws std::invalid_argument x が存在しない場合。
    std::size_t index(const T& x) const {
        auto found = std::find(begin(), end(), x);
        if (found == end()) throw std::invalid_argument("BucketList value not found");
        return static_cast<std::size_t>(std::distance(begin(), found));
    }
    /// x の最初の出現を 1 個削除します。O(N)。
    /// @throws std::invalid_argument x が存在しない場合。
    void remove(const T& x) { pop(static_cast<std::ptrdiff_t>(index(x))); }
    /// バケット構成によらず、要素の並びを operator== で比較します。
    bool operator==(const BucketList& rhs) const { return size_ == rhs.size_ && std::equal(begin(), end(), rhs.begin()); }
    /// 要素の並びが異なるかを比較します。
    bool operator!=(const BucketList& rhs) const { return !(*this == rhs); }
};

} // namespace sorted_set

#endif // SORTED_SET_BUCKET_LIST_HPP
