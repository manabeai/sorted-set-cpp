#pragma once

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
template<class T, bool Multi, class Compare> class SortedCollection;

template<class T> class BucketList {
    template<class, bool, class> friend class SortedCollection;
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
    BucketList() = default;
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
    BucketList(std::initializer_list<T> values) : BucketList(std::vector<T>(values)) {}
    template<class It> BucketList(It first, It last) : BucketList(std::vector<T>(first, last)) {}
    BucketList(const BucketList&) = default;
    BucketList& operator=(const BucketList&) = default;
    BucketList(BucketList&& other) noexcept
        : buckets_(std::move(other.buckets_)), size_(std::exchange(other.size_, 0)) { other.buckets_.clear(); }
    BucketList& operator=(BucketList&& other) noexcept {
        if (this != &other) { buckets_ = std::move(other.buckets_); size_ = std::exchange(other.size_, 0); other.buckets_.clear(); }
        return *this;
    }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    const std::vector<std::vector<T>>& buckets() const noexcept { return buckets_; }
    void clear() noexcept { buckets_.clear(); size_ = 0; }
    const T& at(std::ptrdiff_t i) const { auto [b, j] = locate(i); return buckets_[b][j]; }
    T& at(std::ptrdiff_t i) { auto [b, j] = locate(i); return buckets_[b][j]; }
    const T& operator[](std::ptrdiff_t i) const { return at(i); }
    T& operator[](std::ptrdiff_t i) { return at(i); }
    void append(T value) {
        if (empty()) { buckets_.emplace_back(); buckets_.back().push_back(std::move(value)); size_ = 1; }
        else { const auto b = buckets_.size() - 1; insert_at(b, buckets_[b].size(), std::move(value)); }
    }
    void insert(std::ptrdiff_t i, T value) {
        if (empty()) {
            if (i != 0 && i != -1) throw std::out_of_range("BucketList insert index out of range");
            append(std::move(value));
        } else if (i >= 0 && static_cast<std::size_t>(i) == size_) { append(std::move(value)); }
        else { auto [b, j] = locate(i); insert_at(b, j, std::move(value)); }
    }
    template<class It> void extend(It first, It last) { for (; first != last; ++first) append(*first); }
    T pop(std::ptrdiff_t i = -1) { auto [b, j] = locate(i); return remove_at(b, j); }
    void reverse() {
        std::reverse(buckets_.begin(), buckets_.end());
        for (auto& a : buckets_) std::reverse(a.begin(), a.end());
    }
    BucketList copy() const { return *this; }

    // Mutations invalidate iterators and references. Iteration is read-only.
    class const_iterator {
        const BucketList* owner_ = nullptr;
        std::size_t bucket_ = 0, offset_ = 0;
        friend class BucketList;
        const_iterator(const BucketList* owner, std::size_t bucket, std::size_t offset)
            : owner_(owner), bucket_(bucket), offset_(offset) {}
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        const_iterator() = default;
        reference operator*() const { return owner_->buckets_[bucket_][offset_]; }
        pointer operator->() const { return &**this; }
        const_iterator& operator++() {
            if (++offset_ == owner_->buckets_[bucket_].size()) { ++bucket_; offset_ = 0; }
            return *this;
        }
        const_iterator operator++(int) { auto old = *this; ++*this; return old; }
        const_iterator& operator--() {
            if (offset_ == 0) { --bucket_; offset_ = owner_->buckets_[bucket_].size(); }
            --offset_; return *this;
        }
        const_iterator operator--(int) { auto old = *this; --*this; return old; }
        bool operator==(const const_iterator& rhs) const {
            return owner_ == rhs.owner_ && bucket_ == rhs.bucket_ && offset_ == rhs.offset_;
        }
        bool operator!=(const const_iterator& rhs) const { return !(*this == rhs); }
    };
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    const_iterator begin() const { return {this, 0, 0}; }
    const_iterator end() const { return {this, buckets_.size(), 0}; }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    bool contains(const T& x) const { return std::find(begin(), end(), x) != end(); }
    std::size_t count(const T& x) const { return static_cast<std::size_t>(std::count(begin(), end(), x)); }
    std::size_t index(const T& x) const {
        auto found = std::find(begin(), end(), x);
        if (found == end()) throw std::invalid_argument("BucketList value not found");
        return static_cast<std::size_t>(std::distance(begin(), found));
    }
    void remove(const T& x) { pop(static_cast<std::ptrdiff_t>(index(x))); }
    bool operator==(const BucketList& rhs) const { return size_ == rhs.size_ && std::equal(begin(), end(), rhs.begin()); }
    bool operator!=(const BucketList& rhs) const { return !(*this == rhs); }
};

template<class T, bool Multi, class Compare = std::less<T>> class SortedCollection {
    BucketList<T> data_;
    Compare less_;
    bool equivalent(const T& a, const T& b) const { return !less_(a, b) && !less_(b, a); }
    std::pair<std::size_t, std::size_t> position(const T& x) const {
        std::size_t b = 0;
        while (b + 1 < data_.buckets_.size() && less_(data_.buckets_[b].back(), x)) ++b;
        const auto& a = data_.buckets_[b];
        return {b, static_cast<std::size_t>(std::lower_bound(a.begin(), a.end(), x, less_) - a.begin())};
    }
public:
    SortedCollection() = default;
    explicit SortedCollection(std::vector<T> values, Compare compare = Compare{}) : less_(std::move(compare)) {
        if (!std::is_sorted(values.begin(), values.end(), less_)) std::sort(values.begin(), values.end(), less_);
        if constexpr (!Multi) values.erase(std::unique(values.begin(), values.end(),
            [this](const T& a, const T& b) { return equivalent(a, b); }), values.end());
        data_ = BucketList<T>(std::move(values));
    }
    SortedCollection(std::initializer_list<T> values, Compare compare = Compare{})
        : SortedCollection(std::vector<T>(values), std::move(compare)) {}
    template<class It> SortedCollection(It first, It last, Compare compare = Compare{})
        : SortedCollection(std::vector<T>(first, last), std::move(compare)) {}
    std::size_t size() const noexcept { return data_.size(); }
    bool empty() const noexcept { return data_.empty(); }
    const std::vector<std::vector<T>>& buckets() const noexcept { return data_.buckets(); }
    void clear() noexcept { data_.clear(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
    auto cbegin() const { return data_.cbegin(); }
    auto cend() const { return data_.cend(); }
    auto rbegin() const { return data_.rbegin(); }
    auto rend() const { return data_.rend(); }
    const T& at(std::ptrdiff_t i) const { return data_.at(i); }
    const T& operator[](std::ptrdiff_t i) const { return at(i); }
    T pop(std::ptrdiff_t i = -1) { return data_.pop(i); }
    bool contains(const T& x) const {
        if (empty()) return false;
        auto [b, i] = position(x);
        return i != data_.buckets_[b].size() && equivalent(data_.buckets_[b][i], x);
    }
    bool add(T x) {
        if (empty()) { data_.append(std::move(x)); return true; }
        auto [b, i] = position(x);
        if constexpr (!Multi) {
            if (i != data_.buckets_[b].size() && equivalent(data_.buckets_[b][i], x)) return false;
        }
        data_.insert_at(b, i, std::move(x)); return true;
    }
    bool discard(const T& x) {
        if (empty()) return false;
        auto [b, i] = position(x);
        if (i == data_.buckets_[b].size() || !equivalent(data_.buckets_[b][i], x)) return false;
        data_.remove_at(b, i); return true;
    }
    // nullptr means no matching neighbor. Ordering follows Compare.
    const T* lt(const T& x) const {
        for (auto it = data_.buckets_.rbegin(); it != data_.buckets_.rend(); ++it)
            if (less_(it->front(), x)) return &*std::prev(std::lower_bound(it->begin(), it->end(), x, less_));
        return nullptr;
    }
    const T* le(const T& x) const {
        for (auto it = data_.buckets_.rbegin(); it != data_.buckets_.rend(); ++it)
            if (!less_(x, it->front())) return &*std::prev(std::upper_bound(it->begin(), it->end(), x, less_));
        return nullptr;
    }
    const T* gt(const T& x) const {
        for (const auto& a : data_.buckets_)
            if (less_(x, a.back())) return &*std::upper_bound(a.begin(), a.end(), x, less_);
        return nullptr;
    }
    const T* ge(const T& x) const {
        for (const auto& a : data_.buckets_)
            if (!less_(a.back(), x)) return &*std::lower_bound(a.begin(), a.end(), x, less_);
        return nullptr;
    }
    std::size_t index(const T& x) const {
        std::size_t n = 0;
        for (const auto& a : data_.buckets_) {
            if (!less_(a.back(), x)) return n + static_cast<std::size_t>(std::lower_bound(a.begin(), a.end(), x, less_) - a.begin());
            n += a.size();
        }
        return n;
    }
    std::size_t index_right(const T& x) const {
        std::size_t n = 0;
        for (const auto& a : data_.buckets_) {
            if (less_(x, a.back())) return n + static_cast<std::size_t>(std::upper_bound(a.begin(), a.end(), x, less_) - a.begin());
            n += a.size();
        }
        return n;
    }
    std::size_t count(const T& x) const { return index_right(x) - index(x); }
    bool operator==(const SortedCollection& rhs) const { return data_ == rhs.data_; }
    bool operator!=(const SortedCollection& rhs) const { return !(*this == rhs); }
};
template<class T, class Compare = std::less<T>> using SortedSet = SortedCollection<T, false, Compare>;
template<class T, class Compare = std::less<T>> using SortedMultiset = SortedCollection<T, true, Compare>;
} // namespace sorted_set
